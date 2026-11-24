// -----------------------------------------------------------------------------
// NodeDispatch - Demo SFML (S3.10)
//
// Esqueleto de la capa de presentacion (SDD 6.7): ventana con las tres
// pantallas de la tarjeta (Registrar, Rastrear, Despachar) en estado vacio.
// La CLI sigue siendo la presentacion obligatoria; esto es el extra grafico.
//
// Es opcional: solo se compila con -DBUILD_SFML=ON y vive fuera del nucleo
// (demo/), de modo que include/ y src/ siguen sin SFML, sin STL y sin punto
// flotante. El CI no la construye.
//
// Punto de enganche: la pantalla Despachar ya dibuja la jerarquia real usando
// GeographicHierarchy, porque la fachada (N3) aun no esta en development.
// Cuando entre, cambia solo la fuente de datos (S3.11) y los formularios se
// conectan a registerPackage / trackPackage / dispatchNextPackage.
// -----------------------------------------------------------------------------

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <iostream>
#include <string>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/geographic_hierarchy.hpp"
#include "nodedispatch/types.hpp"

namespace {

constexpr float kNodeRadius = 18.f;
constexpr float kIndentX = 46.f;
constexpr float kRowHeight = 40.f;
constexpr float kTreeOriginX = 110.f;
constexpr float kTreeOriginY = 190.f;
constexpr int kMaxDepth = 32;

const sf::Color kBackground(26, 28, 38);
const sf::Color kPanel(40, 44, 58);
const sf::Color kBorder(88, 96, 120);
const sf::Color kAccent(70, 130, 210);
const sf::Color kText(226, 228, 236);
const sf::Color kMuted(150, 156, 172);

enum class Screen { Register, Track, Dispatch };

// Ultima posicion dibujada en cada profundidad: sirve para trazar la arista
// hacia el padre sin guardar punteros ni usar contenedores.
struct RowAnchor {
    float x = 0.f;
    float y = 0.f;
    bool valid = false;
};

void drawText(sf::RenderWindow& window,
              const sf::Font* font,
              const std::string& value,
              unsigned size,
              float x,
              float y,
              sf::Color color,
              bool centered = false)
{
    if (font == nullptr) {
        return;
    }

    sf::Text text;
    text.setFont(*font);
    text.setString(value);
    text.setCharacterSize(size);
    text.setFillColor(color);

    if (centered) {
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f,
                       bounds.top + bounds.height / 2.f);
    }

    text.setPosition(x, y);
    window.draw(text);
}

// Caja con borde, titulo y una linea de ayuda. Es el "estado vacio" de cada
// control del formulario.
void drawPanel(sf::RenderWindow& window,
               const sf::Font* font,
               float x,
               float y,
               float width,
               float height,
               const std::string& title,
               const std::string& hint)
{
    sf::RectangleShape box(sf::Vector2f(width, height));
    box.setPosition(x, y);
    box.setFillColor(kPanel);
    box.setOutlineColor(kBorder);
    box.setOutlineThickness(1.f);
    window.draw(box);

    drawText(window, font, title, 15, x + 16.f, y + 14.f, kText);
    drawText(window, font, hint, 13, x + 16.f, y + 40.f, kMuted);
}

void drawNode(sf::RenderWindow& window,
              const sf::Font* font,
              const std::string& name,
              float x,
              float y,
              bool isRoot)
{
    sf::CircleShape circle(kNodeRadius);
    circle.setOrigin(kNodeRadius, kNodeRadius);
    circle.setPosition(x, y);
    circle.setFillColor(isRoot ? kAccent : sf::Color(58, 92, 142));
    circle.setOutlineColor(sf::Color(220, 220, 228));
    circle.setOutlineThickness(2.f);
    window.draw(circle);

    drawText(window, font, name, 13, x, y, sf::Color::White, true);
}

void drawLink(sf::RenderWindow& window, float parentX, float parentY, float x, float y)
{
    const sf::Color grey(112, 118, 140);

    sf::Vertex line[2] = {
        sf::Vertex(sf::Vector2f(parentX, parentY), grey),
        sf::Vertex(sf::Vector2f(x, y), grey)
    };

    window.draw(line, 2, sf::Lines);
}

// Arbol indentado: preorden, con la arista al padre tomada del ultimo nodo
// dibujado en la profundidad anterior.
void drawTree(sf::RenderWindow& window,
              const sf::Font* font,
              const nodedispatch::GeographicHierarchy& hierarchy,
              const nodedispatch::Agency& root,
              const Designar::DynArray<nodedispatch::Agency>& descendants)
{
    RowAnchor anchors[kMaxDepth];

    drawNode(window, font, root.name, kTreeOriginX, kTreeOriginY, true);
    anchors[0] = {kTreeOriginX, kTreeOriginY, true};

    for (std::size_t i = 0; i < descendants.size(); ++i) {
        const nodedispatch::Agency& agency = descendants[i];
        const int depth = static_cast<int>(hierarchy.depthOf(agency.id).value_or(0));

        if (depth <= 0 || depth >= kMaxDepth) {
            continue;
        }

        const float x = kTreeOriginX + static_cast<float>(depth) * kIndentX;
        const float y = kTreeOriginY + static_cast<float>(i + 1) * kRowHeight;

        if (anchors[depth - 1].valid) {
            drawLink(window, anchors[depth - 1].x, anchors[depth - 1].y, x, y);
        }

        drawNode(window, font, agency.name, x, y, false);
        anchors[depth] = {x, y, true};
    }
}

void drawTabBar(sf::RenderWindow& window, const sf::Font* font, Screen active)
{
    const char* labels[3] = {"1  Registrar", "2  Rastrear", "3  Despachar"};
    const Screen screens[3] = {Screen::Register, Screen::Track, Screen::Dispatch};

    for (int i = 0; i < 3; ++i) {
        const float x = 30.f + static_cast<float>(i) * 190.f;
        const bool isActive = screens[i] == active;

        sf::RectangleShape tab(sf::Vector2f(180.f, 42.f));
        tab.setPosition(x, 24.f);
        tab.setFillColor(isActive ? kAccent : kPanel);
        tab.setOutlineColor(kBorder);
        tab.setOutlineThickness(1.f);
        window.draw(tab);

        drawText(window, font, labels[i], 15, x + 90.f, 45.f,
                 isActive ? sf::Color::White : kMuted, true);
    }
}

void drawRegisterScreen(sf::RenderWindow& window, const sf::Font* font)
{
    drawPanel(window, font, 30.f, 96.f, 420.f, 90.f,
              "Codigo de rastreo", "Pendiente: registerPackage (formato ND-AAAA-000001)");
    drawPanel(window, font, 30.f, 200.f, 420.f, 90.f,
              "Remitente y destinatario", "Pendiente: clientes registrados por cedula");
    drawPanel(window, font, 30.f, 304.f, 420.f, 90.f,
              "Peso e importe", "Pendiente: peso en kg, importe en centavos");
}

void drawTrackScreen(sf::RenderWindow& window, const sf::Font* font)
{
    drawPanel(window, font, 30.f, 96.f, 420.f, 90.f,
              "Codigo de rastreo", "Pendiente: trackPackage");
    drawPanel(window, font, 30.f, 200.f, 480.f, 300.f,
              "Historial de estados", "Pendiente: linea de tiempo con fecha y agencia");
}

void drawDispatchScreen(sf::RenderWindow& window,
                        const sf::Font* font,
                        const nodedispatch::GeographicHierarchy& hierarchy,
                        const nodedispatch::Agency& root,
                        const Designar::DynArray<nodedispatch::Agency>& descendants)
{
    drawText(window, font, "Agencias registradas", 16, 30.f, 96.f, kText);
    drawTree(window, font, hierarchy, root, descendants);

    drawPanel(window, font, 620.f, 96.f, 360.f, 420.f,
              "Cola de despacho",
              "Pendiente S3.11: dispatchNextPackage por agencia");
}

} // namespace

int main()
{
    using nodedispatch::Agency;
    using nodedispatch::GeographicHierarchy;

    // Jerarquia de demostracion: ciudad -> zona -> acopio.
    GeographicHierarchy hierarchy;

    struct SeedAgency {
        const char* id;
        const char* name;
        const char* parentId;
    };

    const SeedAgency seeds[5] = {
        {"AG-CCS", "Caracas", ""},
        {"AG-CCS-CH", "Chacao", "AG-CCS"},
        {"AG-CCS-BA", "Baruta", "AG-CCS"},
        {"AG-CCS-BA-CF", "El Cafetal", "AG-CCS-BA"},
        {"AG-VAL", "Valencia", "AG-CCS"}
    };

    for (const SeedAgency& seed : seeds) {
        Agency agency;
        agency.id = seed.id;
        agency.name = seed.name;
        agency.parentId = seed.parentId;
        agency.location.state = "Miranda";
        agency.location.city = seed.name;
        agency.location.exactAddress = std::string("Sede ") + seed.name;
        agency.location.postalCode = "1000";

        if (hierarchy.addAgency(agency, seed.parentId) != nodedispatch::OperationResult::OK) {
            std::cerr << "Aviso: la jerarquia rechazo la agencia " << seed.id << '\n';
        }
    }

    const auto root = hierarchy.root();

    if (!root.has_value()) {
        std::cerr << "Sin agencias registradas: nada que dibujar.\n";
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode(1024, 700),
                            "NodeDispatch - nodedispatch_gui (S3.10)");
    window.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");

    if (!hasFont) {
        hasFont = font.loadFromFile(
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    }

    if (!hasFont) {
        std::cerr << "Aviso: sin fuente TTF; el texto no se dibujara.\n";
    }

    const sf::Font* fontPtr = hasFont ? &font : nullptr;

    // Copia estable de los descendientes: la jerarquia es de solo insercion.
    const Designar::DynArray<Agency> descendants = hierarchy.descendants(root->id);

    Screen screen = Screen::Register;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type != sf::Event::KeyPressed) {
                continue;
            }

            switch (event.key.code) {
            case sf::Keyboard::Escape:
                window.close();
                break;
            case sf::Keyboard::Num1: screen = Screen::Register; break;
            case sf::Keyboard::Num2: screen = Screen::Track;    break;
            case sf::Keyboard::Num3: screen = Screen::Dispatch; break;
            case sf::Keyboard::Tab:
                screen = (screen == Screen::Register) ? Screen::Track
                       : (screen == Screen::Track)    ? Screen::Dispatch
                                                      : Screen::Register;
                break;
            default:
                break;
            }
        }

        window.clear(kBackground);

        drawText(window, fontPtr, "NodeDispatch", 20, 30.f, 60.f, kText);
        drawText(window, fontPtr,
                 std::to_string(hierarchy.size()) + " agencias | 1/2/3 o Tab para cambiar | ESC para salir",
                 13, 620.f, 62.f, kMuted);

        drawTabBar(window, fontPtr, screen);

        switch (screen) {
        case Screen::Register:
            drawRegisterScreen(window, fontPtr);
            break;
        case Screen::Track:
            drawTrackScreen(window, fontPtr);
            break;
        case Screen::Dispatch:
            drawDispatchScreen(window, fontPtr, hierarchy, *root, descendants);
            break;
        }

        window.display();
    }

    return 0;
}
