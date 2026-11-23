// -----------------------------------------------------------------------------
// NodeDispatch - S2.3 Inventario por agencia sobre RbTree
//
// El inventario guarda solo codigos: la prueba comprueba el comportamiento que
// ve la fachada (alta, baja, pertenencia y orden) y no los invariantes internos
// del arbol rojo-negro, que la API publica de RbTree no expone.
// -----------------------------------------------------------------------------

#include <string>

#include <cstdint>

#include <doctest/doctest.h>

#include "nodedispatch/inventory_index.hpp"
#include "nodedispatch/types.hpp"

using namespace nodedispatch;

namespace {

std::string code(int i)
{
    // Relleno a la izquierda para que el orden lexicografico coincida con el
    // numerico y el test sea legible.
    std::string n = std::to_string(i);
    return "ND-" + std::string(n.size() < 4 ? 4 - n.size() : 0, '0') + n;
}

} // namespace

TEST_CASE("InventoryIndex da de alta, encuentra y rechaza codigos repetidos")
{
    InventoryIndex inventory;

    CHECK(inventory.add("AG-CCS", "ND-0001") == OperationResult::OK);
    CHECK(inventory.contains("AG-CCS", "ND-0001"));
    CHECK(inventory.size("AG-CCS") == 1);

    // Mismo codigo, misma agencia: duplicado.
    CHECK(inventory.add("AG-CCS", "ND-0001") == OperationResult::DUPLICATE);
    CHECK(inventory.size("AG-CCS") == 1);

    // Mismo codigo, otra agencia: se permite (traslado entre nodos).
    CHECK(inventory.add("AG-MRD", "ND-0001") == OperationResult::OK);
    CHECK(inventory.size("AG-MRD") == 1);
    CHECK(inventory.agencyCount() == 2);

    // Codigo o agencia vacios.
    CHECK(inventory.add("AG-CCS", "") == OperationResult::INVALID_INPUT);
    CHECK(inventory.add("", "ND-0002") == OperationResult::INVALID_INPUT);
}

TEST_CASE("InventoryIndex elimina y distingue agencia inexistente de codigo ausente")
{
    InventoryIndex inventory;
    inventory.add("AG-CCS", "ND-0001");
    inventory.add("AG-CCS", "ND-0002");

    CHECK(inventory.remove("AG-CCS", "ND-0001"));
    CHECK_FALSE(inventory.contains("AG-CCS", "ND-0001"));
    CHECK(inventory.size("AG-CCS") == 1);

    // Ya no esta.
    CHECK_FALSE(inventory.remove("AG-CCS", "ND-0001"));

    // Agencia que nunca existio: ni alta ni pertenencia.
    CHECK_FALSE(inventory.remove("AG-NO-EXISTE", "ND-0001"));
    CHECK_FALSE(inventory.contains("AG-NO-EXISTE", "ND-0001"));
    CHECK(inventory.size("AG-NO-EXISTE") == 0);
    CHECK(inventory.is_empty("AG-NO-EXISTE"));
}

TEST_CASE("InventoryIndex devuelve los codigos ordenados aunque se inserten al reves")
{
    InventoryIndex inventory;

    // Insercion descendente: si el recorrido no fuera in-order, saldrian al
    // reves y el test fallaria.
    for (int i = 100; i >= 1; --i)
    {
        CHECK(inventory.add("AG-CCS", code(i)) == OperationResult::OK);
    }

    const Designar::DynArray<std::string> sorted = inventory.listSorted("AG-CCS");
    REQUIRE(sorted.size() == 100);

    for (int i = 1; i <= 100; ++i)
    {
        CHECK(sorted[i - 1] == code(i));
    }

    // Una agencia vacia devuelve una lista vacia, no un fallo.
    CHECK(inventory.listSorted("AG-VACIA").is_empty());
}

TEST_CASE("InventoryIndex sostiene tres agencias con cien codigos cada una")
{
    InventoryIndex inventory;
    const char* agencies[] = {"AG-CCS", "AG-MRD", "AG-VLN"};

    for (const char* agency : agencies)
    {
        for (int i = 1; i <= 100; ++i)
        {
            REQUIRE(inventory.add(agency, code(i)) == OperationResult::OK);
        }
    }

    CHECK(inventory.agencyCount() == 3);

    for (const char* agency : agencies)
    {
        CHECK(inventory.size(agency) == 100);
        CHECK(inventory.contains(agency, code(50)));

        const Designar::DynArray<std::string> sorted = inventory.listSorted(agency);
        REQUIRE(sorted.size() == 100);
        CHECK(sorted[0] == code(1));
        CHECK(sorted[99] == code(100));
    }

    // La baja en una agencia no toca a las demas.
    CHECK(inventory.remove("AG-MRD", code(50)));
    CHECK_FALSE(inventory.contains("AG-MRD", code(50)));
    CHECK(inventory.contains("AG-CCS", code(50)));
    CHECK(inventory.contains("AG-VLN", code(50)));
}

TEST_CASE("InventoryIndex libera todo con clear y queda reutilizable")
{
    InventoryIndex inventory;
    inventory.add("AG-CCS", "ND-0001");
    inventory.add("AG-MRD", "ND-0002");

    inventory.clear();

    CHECK(inventory.agencyCount() == 0);
    CHECK_FALSE(inventory.contains("AG-CCS", "ND-0001"));
    CHECK(inventory.listSorted("AG-CCS").is_empty());

    // Se puede volver a usar despues de clear().
    CHECK(inventory.add("AG-CCS", "ND-0003") == OperationResult::OK);
    CHECK(inventory.size("AG-CCS") == 1);
}


TEST_CASE("InventoryIndex conserva los invariantes rojo-negro (S2.4)")
{
    InventoryIndex index;

    // Indice vacio: no hay arboles que verificar.
    CHECK(index.verify());

    // Generador congruencial lineal: determinista y sin <random>.
    std::uint32_t state = 12345U;
    auto next = [&state]() {
        state = state * 1664525U + 1013904223U;
        return state;
    };

    const char* agencies[] = {"AG-CCS", "AG-MAR", "AG-VAL"};

    for (int i = 0; i < 5000; ++i)
    {
        const std::string code = "ND-" + std::to_string(100000 + (next() % 2000));
        const std::string agency = agencies[next() % 3];

        // Con 2000 codigos y 5000 altas hay repetidos: DUPLICATE es esperado.
        const OperationResult result = index.add(agency, code);
        REQUIRE((result == OperationResult::OK || result == OperationResult::DUPLICATE));
    }

    // Tras 5000 altas con codigos repetidos: los arboles siguen equilibrados.
    CHECK(index.verify());

    for (int i = 0; i < 1500; ++i)
    {
        const std::string code = "ND-" + std::to_string(100000 + (next() % 2000));
        const std::string agency = agencies[next() % 3];
        index.remove(agency, code); // puede no existir: no importa
    }

    CHECK(index.verify());

    // Y el orden in-order sigue siendo ascendente en cada agencia.
    for (const char* agency : agencies)
    {
        const auto codes = index.listSorted(agency);
        for (std::size_t i = 1; i < codes.size(); ++i)
        {
            CHECK(codes[i - 1] < codes[i]);
        }
    }

    index.clear();
    CHECK(index.verify());
}
