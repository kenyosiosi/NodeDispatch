// -----------------------------------------------------------------------------
// NodeDispatch - S2.2 Pruebas de GeographicHierarchy (doctest + CTest)
//
// Cubre: raiz unica, IDs duplicados, padre inexistente, normalizacion del
// parentId, busqueda O(1), DFS, BFS por niveles, profundidad, copias seguras,
// agencia inexistente, cadena de 1000 agencias y clear/reutilizacion.
//
// Seis TEST_CASE con nombre unico: doctest_discover_tests registra solo el
// nombre del caso, sin TEST_SUITE, y dos homonimos colisionarian en CTest.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <initializer_list>
#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/geographic_hierarchy.hpp"

using namespace nodedispatch;

namespace {

Agency agency(const std::string& id, const std::string& name,
              const std::string& parentId = "")
{
    Agency a;
    a.id = id;
    a.name = name;
    a.location = Address{"Tachira", "San Cristobal", "Calle 1", "5001"};
    a.parentId = parentId;
    return a;
}

bool sameIds(const Designar::DynArray<Agency>& list,
             std::initializer_list<std::string> expected)
{
    if (list.size() != expected.size())
    {
        return false;
    }

    std::size_t i = 0;
    for (const std::string& id : expected)
    {
        if (list[i++].id != id)
        {
            return false;
        }
    }

    return true;
}

// GeographicHierarchy no es copiable (RAII): se puebla por referencia.
void seedSmallHierarchy(GeographicHierarchy& h)
{
    REQUIRE(h.addAgency(agency("AG-000", "Centro"), "") == OperationResult::OK);
    REQUIRE(h.addAgency(agency("AG-001", "Norte"), "AG-000") == OperationResult::OK);
    REQUIRE(h.addAgency(agency("AG-002", "Sur"), "AG-000") == OperationResult::OK);
    REQUIRE(h.addAgency(agency("AG-003", "Acopio Norte"), "AG-001") == OperationResult::OK);
    REQUIRE(h.addAgency(agency("AG-004", "Acopio Norte 2"), "AG-001") == OperationResult::OK);
    REQUIRE(h.addAgency(agency("AG-005", "Acopio Sur"), "AG-002") == OperationResult::OK);
}

} // namespace

TEST_CASE("jerarquia vacia, insercion y validacion de agencias (S2.2)")
{
    GeographicHierarchy h;

    CHECK(h.size() == 0);
    CHECK_FALSE(h.contains("AG-000"));
    CHECK_FALSE(h.root().has_value());
    CHECK_FALSE(h.findAgency("AG-000").has_value());
    CHECK_FALSE(h.parentOf("AG-000").has_value());
    CHECK_FALSE(h.depthOf("AG-000").has_value());
    CHECK(h.descendants("AG-000").is_empty());
    CHECK(h.levelOrder("AG-000").is_empty());

    CHECK(h.addAgency(agency("AG-000", "Centro"), "") == OperationResult::OK);
    CHECK(h.size() == 1);

    // ID duplicado.
    CHECK(h.addAgency(agency("AG-000", "Duplicada"), "") ==
          OperationResult::DUPLICATE);

    // Segunda raiz.
    CHECK(h.addAgency(agency("AG-999", "Otra raiz"), "") ==
          OperationResult::INVALID_STATE);

    // ID vacio.
    CHECK(h.addAgency(agency("", "Sin ID"), "AG-000") ==
          OperationResult::INVALID_INPUT);

    // Padre inexistente.
    CHECK(h.addAgency(agency("AG-001", "Norte"), "AG-404") ==
          OperationResult::NOT_FOUND);

    // Hijo valido: el parentId almacenado se normaliza al argumento.
    const Agency child = agency("AG-001", "Norte", "VALOR-IGNORADO");
    CHECK(h.addAgency(child, "AG-000") == OperationResult::OK);
    CHECK(h.size() == 2);

    auto stored = h.findAgency("AG-001");
    REQUIRE(stored.has_value());
    CHECK(stored->parentId == "AG-000");

    // Las copias son independientes del estado interno.
    stored->name = "Mutada por el cliente";
    CHECK(h.findAgency("AG-001")->name == "Norte");
}

TEST_CASE("busqueda por id, padre y profundidad (S2.2)")
{
    GeographicHierarchy h;
    seedSmallHierarchy(h);

    CHECK(h.contains("AG-003"));
    CHECK_FALSE(h.contains("AG-404"));

    const auto root = h.root();
    REQUIRE(root.has_value());
    CHECK(root->id == "AG-000");
    CHECK(root->parentId.empty());

    const auto parent = h.parentOf("AG-003");
    REQUIRE(parent.has_value());
    CHECK(parent->id == "AG-001");
    CHECK_FALSE(h.parentOf("AG-000").has_value());

    REQUIRE(h.depthOf("AG-000").has_value());
    CHECK(h.depthOf("AG-000").value() == 0);
    REQUIRE(h.depthOf("AG-001").has_value());
    CHECK(h.depthOf("AG-001").value() == 1);
    REQUIRE(h.depthOf("AG-003").has_value());
    CHECK(h.depthOf("AG-003").value() == 2);
    CHECK_FALSE(h.depthOf("AG-404").has_value());
}

TEST_CASE("recorrido DFS de descendientes (S2.2)")
{
    GeographicHierarchy h;
    seedSmallHierarchy(h);

    // Preorden: AG-001, AG-003, AG-004, AG-002, AG-005.
    const auto dfs = h.descendants("AG-000");
    CHECK(sameIds(dfs, {"AG-001", "AG-003", "AG-004", "AG-002", "AG-005"}));
    CHECK(dfs.size() == 5);

    // El subarbol de AG-001 no incluye a los hijos de AG-002.
    CHECK(sameIds(h.descendants("AG-001"), {"AG-003", "AG-004"}));
}

TEST_CASE("recorrido BFS por niveles (S2.2)")
{
    GeographicHierarchy h;
    seedSmallHierarchy(h);

    // levelOrder usa Designar::ListQueue: nivel 1 antes que nivel 2.
    const auto bfs = h.levelOrder("AG-000");
    REQUIRE(bfs.size() == 6);
    CHECK(bfs[0].id == "AG-000");
    CHECK(bfs[1].id == "AG-001");
    CHECK(bfs[2].id == "AG-002");
    CHECK(bfs[3].id == "AG-003");
    CHECK(bfs[4].id == "AG-004");
    CHECK(bfs[5].id == "AG-005");

    CHECK(sameIds(h.levelOrder("AG-003"), {"AG-003"}));
}

TEST_CASE("agencia inexistente y reutilizacion tras clear (S2.2)")
{
    GeographicHierarchy h;
    seedSmallHierarchy(h);

    // Consultar un ID que nunca existio no rompe: devuelve listas vacias.
    CHECK(h.descendants("AG-404").is_empty());
    CHECK(h.levelOrder("AG-404").is_empty());
    CHECK_FALSE(h.contains("AG-404"));

    h.clear();

    CHECK(h.size() == 0);
    CHECK_FALSE(h.root().has_value());
    CHECK_FALSE(h.contains("AG-000"));
    CHECK_FALSE(h.contains("AG-001"));

    CHECK(h.addAgency(agency("AG-100", "Nueva raiz"), "") == OperationResult::OK);
    CHECK(h.size() == 1);
    REQUIRE(h.root().has_value());
    CHECK(h.root()->id == "AG-100");
}

TEST_CASE("cadena de 1000 agencias (S2.2)")
{
    GeographicHierarchy h;

    constexpr int N = 1000;
    REQUIRE(h.addAgency(agency("AG-10000", "Nodo 0"), "") == OperationResult::OK);

    for (int i = 1; i < N; ++i)
    {
        const std::string id = "AG-" + std::to_string(10000 + i);
        const std::string parent = "AG-" + std::to_string(10000 + i - 1);

        REQUIRE(h.addAgency(agency(id, "Nodo " + std::to_string(i)), parent) ==
                OperationResult::OK);
    }

    CHECK(h.size() == static_cast<std::size_t>(N));
    REQUIRE(h.depthOf("AG-10999").has_value());
    CHECK(h.depthOf("AG-10999").value() == static_cast<std::size_t>(N - 1));
    CHECK(h.descendants("AG-10000").size() == static_cast<std::size_t>(N - 1));
    CHECK(h.levelOrder("AG-10000").size() == static_cast<std::size_t>(N));
}
