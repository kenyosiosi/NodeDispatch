// -----------------------------------------------------------------------------
// NodeDispatch - CLI de comandos (S3.11)
//
// Presentacion obligatoria del MVP (SDD 6.7). Lee ordenes de un guion (stdin o
// --script <archivo>) y las ejecuta contra la fachada NodeDispatch. Pensada
// para que `scripts/demo.sh` produzca un transcript reproducible.
//
// Gramatica: una orden por linea, tokens separados por espacios, '#' comenta.
//
//   agency add <id> <parentId|-> <nombre>
//   agency tree <id>
//   client add <cedula> <nombre> <apellido>
//   package register <codigo> <agencia> <remitente> <destinatario>
//                    <baseCents> <PREPAID|COD> <HOME|AGENCY> [<nivel>]
//   package track <codigo>
//   package pay <codigo> <PAID|PENDING|FAILED>
//   package status <codigo> <ESTADO>
//   package cancel <codigo>
//   package dispatch <agencia>
//   report revenue
//   report status <ESTADO>
//   report agency <agencia>
//   save <ruta>      (S4: la persistencia real aun no existe)
//   load <ruta>      (S4)
//   help
//   quit
//
// Los nombres no llevan espacios (la demo usa ciudades de una palabra).
// -----------------------------------------------------------------------------

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "nodedispatch/node_dispatch.hpp"
#include "nodedispatch/types.hpp"

namespace {

using namespace nodedispatch;

bool parsePaymentType(const std::string& token, PaymentType& out)
{
    if (token == "PREPAID")
    {
        out = PaymentType::PREPAID;
        return true;
    }
    if (token == "COD" || token == "CASH_ON_DELIVERY")
    {
        out = PaymentType::CASH_ON_DELIVERY;
        return true;
    }
    return false;
}

bool parseDeliveryMethod(const std::string& token, DeliveryMethod& out)
{
    if (token == "HOME" || token == "HOME_DELIVERY")
    {
        out = DeliveryMethod::HOME_DELIVERY;
        return true;
    }
    if (token == "AGENCY" || token == "AGENCY_PICKUP")
    {
        out = DeliveryMethod::AGENCY_PICKUP;
        return true;
    }
    return false;
}

bool parseServiceLevel(const std::string& token, ServiceLevel& out)
{
    if (token == "EXPRESS" || token == "EXPRESS_24H")
    {
        out = ServiceLevel::EXPRESS_24H;
        return true;
    }
    if (token == "PRIORITY")
    {
        out = ServiceLevel::PRIORITY;
        return true;
    }
    if (token == "STANDARD")
    {
        out = ServiceLevel::STANDARD;
        return true;
    }
    return false;
}

bool parsePaymentStatus(const std::string& token, PaymentStatus& out)
{
    if (token == "PAID")
    {
        out = PaymentStatus::PAID;
        return true;
    }
    if (token == "PENDING")
    {
        out = PaymentStatus::PENDING;
        return true;
    }
    if (token == "FAILED")
    {
        out = PaymentStatus::FAILED;
        return true;
    }
    return false;
}

bool parsePackageStatus(const std::string& token, PackageStatus& out)
{
    if (token == "REGISTERED")
    {
        out = PackageStatus::REGISTERED;
        return true;
    }
    if (token == "IN_TRANSIT")
    {
        out = PackageStatus::IN_TRANSIT;
        return true;
    }
    if (token == "AT_LOCAL_AGENCY")
    {
        out = PackageStatus::AT_LOCAL_AGENCY;
        return true;
    }
    if (token == "OUT_FOR_DELIVERY")
    {
        out = PackageStatus::OUT_FOR_DELIVERY;
        return true;
    }
    if (token == "DELIVERED")
    {
        out = PackageStatus::DELIVERED;
        return true;
    }
    if (token == "RETURNED")
    {
        out = PackageStatus::RETURNED;
        return true;
    }
    return false;
}

const char* nameOf(PackageStatus status)
{
    switch (status)
    {
    case PackageStatus::REGISTERED:        return "REGISTERED";
    case PackageStatus::IN_TRANSIT:       return "IN_TRANSIT";
    case PackageStatus::AT_LOCAL_AGENCY:  return "AT_LOCAL_AGENCY";
    case PackageStatus::OUT_FOR_DELIVERY: return "OUT_FOR_DELIVERY";
    case PackageStatus::DELIVERED:        return "DELIVERED";
    case PackageStatus::RETURNED:         return "RETURNED";
    }
    return "DESCONOCIDO";
}

const char* nameOf(PaymentStatus status)
{
    switch (status)
    {
    case PaymentStatus::PENDING: return "PENDING";
    case PaymentStatus::PAID:    return "PAID";
    case PaymentStatus::FAILED:  return "FAILED";
    }
    return "DESCONOCIDO";
}

void printPackage(std::ostream& out, const Package& package)
{
    out << "paquete " << package.trackingCode
        << " estado=" << nameOf(package.status)
        << " pago=" << nameOf(package.paymentStatus)
        << " agencia=" << package.targetAgencyId
        << " totalCents=" << (package.baseCents + package.deliverySurchargeCents)
        << '\n';
}

// Profundidad de la agencia contando padres con findAgency (copia segura).
std::size_t depthOf(const NodeDispatch& app, const std::string& id)
{
    std::size_t depth = 0;
    std::string current = id;

    while (!current.empty())
    {
        const std::optional<Agency> agency = app.findAgency(current);
        if (!agency.has_value())
        {
            break;
        }
        current = agency->parentId;
        if (!current.empty())
        {
            ++depth;
        }
    }

    return depth;
}

void printHelp(std::ostream& out)
{
    out << "ordenes:\n"
        << "  agency add <id> <padre|-> <nombre>\n"
        << "  agency tree <id>\n"
        << "  client add <cedula> <nombre> <apellido>\n"
        << "  package register <codigo> <agencia> <remitente> <destinatario>"
           " <baseCents> <PREPAID|COD> <HOME|AGENCY> [<nivel>]\n"
        << "  package track <codigo>\n"
        << "  package pay <codigo> <PAID|PENDING|FAILED>\n"
        << "  package status <codigo> <ESTADO>\n"
        << "  package cancel <codigo>\n"
        << "  package dispatch <agencia>\n"
        << "  report revenue | status <ESTADO> | agency <agencia>\n"
        << "  save <ruta> / load <ruta>   (pendiente S4)\n"
        << "  quit\n";
}

// --- Comandos ----------------------------------------------------------------

void cmdAgency(NodeDispatch& app, std::istringstream& rest, std::ostream& out)
{
    std::string action;
    rest >> action;

    if (action == "add")
    {
        std::string id;
        std::string parentToken;
        std::string name;
        rest >> id >> parentToken >> name;

        Agency agency;
        agency.id = id;
        agency.name = name;
        agency.parentId = parentToken == "-" ? "" : parentToken;
        agency.location.state = "N/A";
        agency.location.city = name;
        agency.location.exactAddress = "Sede " + name;
        agency.location.postalCode = "0000";

        const bool ok = app.addAgency(agency, agency.parentId);
        out << (ok ? "OK " : "ERROR ") << "agency add " << id << '\n';
        return;
    }

    if (action == "tree")
    {
        std::string id;
        rest >> id;

        const std::optional<Agency> root = app.findAgency(id);
        if (!root.has_value())
        {
            out << "ERROR agency tree " << id << " (no existe)\n";
            return;
        }

        out << id << " 0 " << root->name << '\n';
        const Designar::DynArray<Agency> descendants = app.listDescendants(id);
        for (std::size_t i = 0; i < descendants.size(); ++i)
        {
            out << descendants[i].id << ' ' << depthOf(app, descendants[i].id)
                << ' ' << descendants[i].name << '\n';
        }
        return;
    }

    out << "ERROR agency: accion desconocida\n";
}

void cmdClient(NodeDispatch& app, std::istringstream& rest, std::ostream& out)
{
    std::string action;
    rest >> action;

    if (action != "add")
    {
        out << "ERROR client: accion desconocida\n";
        return;
    }

    Client client;
    rest >> client.nationalId >> client.firstName >> client.lastName;
    client.phone = "+58-000-0000000";
    client.address.state = "N/A";
    client.address.city = "N/A";
    client.address.exactAddress = "N/A";
    client.address.postalCode = "0000";

    const bool ok = app.registerClient(client);
    out << (ok ? "OK " : "ERROR ") << "client add " << client.nationalId << '\n';
}

void cmdPackageRegister(NodeDispatch& app, std::istringstream& rest, std::ostream& out)
{
    Package package;
    std::string paymentToken;
    std::string methodToken;
    std::string levelToken;
    std::string baseCents;

    rest >> package.trackingCode >> package.targetAgencyId
         >> package.senderId >> package.receiverId
         >> baseCents >> paymentToken >> methodToken >> levelToken;

    package.baseCents = std::stoll(baseCents);

    if (!parsePaymentType(paymentToken, package.paymentType))
    {
        out << "ERROR package register: tipo de pago invalido\n";
        return;
    }

    if (!parseDeliveryMethod(methodToken, package.deliveryMethod))
    {
        out << "ERROR package register: metodo de entrega invalido\n";
        return;
    }

    package.deliverySurchargeCents =
        package.deliveryMethod == DeliveryMethod::HOME_DELIVERY ? 300 : 0;

    package.serviceLevel = ServiceLevel::STANDARD;

    if (!levelToken.empty() && !parseServiceLevel(levelToken, package.serviceLevel))
    {
        out << "ERROR package register: nivel de servicio invalido\n";
        return;
    }

    package.weightKg = 1.0;
    package.status = PackageStatus::REGISTERED;

    const bool ok = app.registerPackage(package);
    out << (ok ? "OK " : "ERROR ") << "package register " << package.trackingCode << '\n';
}

void cmdPackage(NodeDispatch& app, std::istringstream& rest, std::ostream& out)
{
    std::string action;
    rest >> action;

    if (action == "register")
    {
        cmdPackageRegister(app, rest, out);
        return;
    }

    if (action == "track")
    {
        std::string code;
        rest >> code;

        const std::optional<Package> package = app.trackPackage(code);
        if (!package.has_value())
        {
            out << "ERROR package track " << code << " (no existe)\n";
            return;
        }

        printPackage(out, *package);
        return;
    }

    if (action == "pay")
    {
        std::string code;
        std::string token;
        rest >> code >> token;

        PaymentStatus status = PaymentStatus::PENDING;
        if (!parsePaymentStatus(token, status))
        {
            out << "ERROR package pay: estado invalido\n";
            return;
        }

        const bool ok = app.updatePaymentStatus(code, status);
        out << (ok ? "OK " : "ERROR ") << "package pay " << code << ' '
            << nameOf(status) << '\n';
        return;
    }

    if (action == "status")
    {
        std::string code;
        std::string token;
        rest >> code >> token;

        PackageStatus status = PackageStatus::REGISTERED;
        if (!parsePackageStatus(token, status))
        {
            out << "ERROR package status: estado invalido\n";
            return;
        }

        const bool ok = app.updatePackageStatus(code, status);
        out << (ok ? "OK " : "ERROR ") << "package status " << code << ' '
            << nameOf(status) << '\n';
        return;
    }

    if (action == "cancel")
    {
        std::string code;
        rest >> code;

        const bool ok = app.cancelPackage(code);
        out << (ok ? "OK " : "ERROR ") << "package cancel " << code << '\n';
        return;
    }

    if (action == "dispatch")
    {
        std::string agencyId;
        rest >> agencyId;

        const std::optional<Package> dispatched = app.dispatchNextPackage(agencyId);
        if (!dispatched.has_value())
        {
            out << "ERROR package dispatch " << agencyId << " (nada elegible)\n";
            return;
        }

        printPackage(out, *dispatched);
        return;
    }

    out << "ERROR package: accion desconocida\n";
}

void cmdReport(NodeDispatch& app, std::istringstream& rest, std::ostream& out)
{
    std::string action;
    rest >> action;

    if (action == "revenue")
    {
        out << "ingresosCents=" << app.totalRevenueCents() << '\n';
        return;
    }

    if (action == "status")
    {
        std::string token;
        rest >> token;

        PackageStatus status = PackageStatus::REGISTERED;
        if (!parsePackageStatus(token, status))
        {
            out << "ERROR report status: estado invalido\n";
            return;
        }

        const Designar::DynArray<Package> packages = app.packagesByStatus(status);
        out << "estado=" << nameOf(status) << " total=" << packages.size() << '\n';
        for (std::size_t i = 0; i < packages.size(); ++i)
        {
            printPackage(out, packages[i]);
        }
        return;
    }

    if (action == "agency")
    {
        std::string agencyId;
        rest >> agencyId;

        const Designar::DynArray<Package> packages = app.packagesAtAgency(agencyId);
        out << "agencia=" << agencyId << " total=" << packages.size() << '\n';
        for (std::size_t i = 0; i < packages.size(); ++i)
        {
            printPackage(out, packages[i]);
        }
        return;
    }

    out << "ERROR report: accion desconocida\n";
}

// Devuelve false cuando la orden es `quit`.
bool runCommand(NodeDispatch& app, const std::string& line, std::ostream& out)
{
    std::istringstream input(line);
    std::string command;
    input >> command;

    if (command.empty() || command[0] == '#')
    {
        return true;
    }

    if (command == "help")
    {
        printHelp(out);
        return true;
    }

    if (command == "quit")
    {
        return false;
    }

    if (command == "agency")
    {
        cmdAgency(app, input, out);
        return true;
    }

    if (command == "client")
    {
        cmdClient(app, input, out);
        return true;
    }

    if (command == "package")
    {
        cmdPackage(app, input, out);
        return true;
    }

    if (command == "report")
    {
        cmdReport(app, input, out);
        return true;
    }

    if (command == "save" || command == "load")
    {
        // Stubs de N3: el contrato existe, la persistencia real es S4.
        out << "ERROR " << command << " no implementado (pendiente S4)\n";
        return true;
    }

    out << "ERROR comando desconocido: " << command << '\n';
    return true;
}

int runScript(std::istream& in, std::ostream& out)
{
    NodeDispatch app;
    std::string line;

    while (std::getline(in, line))
    {
        out << "> " << line << '\n';
        if (!runCommand(app, line, out))
        {
            break;
        }
    }

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 3 && std::string(argv[1]) == "--script")
    {
        std::ifstream file(argv[2]);
        if (!file)
        {
            std::cerr << "No se pudo abrir el guion: " << argv[2] << '\n';
            return 1;
        }

        return runScript(file, std::cout);
    }

    if (argc > 1)
    {
        std::cerr << "Uso: nodedispatch_cli [--script <archivo>]\n";
        return 1;
    }

    return runScript(std::cin, std::cout);
}
