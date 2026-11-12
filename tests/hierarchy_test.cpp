// -----------------------------------------------------------------------------
// NodeDispatch - S2.1 Pruebas de GeographicHierarchy
//
// Cubre: raiz unica, IDs duplicados, padre inexistente, normalizacion del
// parentId, busqueda O(1), DFS, BFS, profundidad, copias seguras y liberacion
// de memoria (clear / destructor).
// -----------------------------------------------------------------------------

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>

#include "nodedispatch/geographic_hierarchy.hpp"

using namespace nodedispatch;

namespace {

int failures = 0;

void check(bool ok, const char* expr, int line)
{
    if (!ok)
    {
        std::cerr << "  [FAIL] " << expr << "  (linea " << line << ")\n";
        ++failures;
    }
}

#define CHECK(expr) check((expr), #expr, __LINE__)

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

void testEmptyHierarchy()
{
    std::cout << "- jerarquia vacia\n";
    GeographicHierarchy h;

    CHECK(h.size() == 0);
    CHECK(!h.contains("AG-000"));
    CHECK(!h.root().has_value());
    CHECK(!h.findAgency("AG-000").has_value());
    CHECK(!h.parentOf("AG-000").has_value());
    CHECK(!h.depthOf("AG-000").has_value());
    CHECK(h.descendants("AG-000").is_empty());
    CHECK(h.levelOrder("AG-000").is_empty());
}

void testInsertionAndValidation()
{
    std::cout << "- insercion y validacion\n";
    GeographicHierarchy h;

    CHECK(h.addAgency(agency("AG-000", "Centro"), "") ==
          OperationResult::OK);
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
    Agency child = agency("AG-001", "Norte", "VALOR-IGNORADO");
    CHECK(h.addAgency(child, "AG-000") == OperationResult::OK);
    CHECK(h.size() == 2);

    auto stored = h.findAgency("AG-001");
    CHECK(stored.has_value());
    CHECK(stored->parentId == "AG-000");

    // Las copias son independientes del estado interno.
    stored->name = "Mutada por el cliente";
    CHECK(h.findAgency("AG-001")->name == "Norte");
}

void testNavigation()
{
    std::cout << "- navegacion (find, parent, depth)\n";
    GeographicHierarchy h;

    CHECK(h.addAgency(agency("AG-000", "Centro"), "") == OperationResult::OK);
    CHECK(h.addAgency(agency("AG-001", "Norte"), "AG-000") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-002", "Sur"), "AG-000") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-003", "Acopio Norte"), "AG-001") ==
          OperationResult::OK);

    CHECK(h.contains("AG-003"));
    CHECK(!h.contains("AG-404"));

    auto root = h.root();
    CHECK(root.has_value());
    CHECK(root->id == "AG-000");
    CHECK(root->parentId.empty());

    auto parent = h.parentOf("AG-003");
    CHECK(parent.has_value());
    CHECK(parent->id == "AG-001");
    CHECK(!h.parentOf("AG-000").has_value());

    CHECK(h.depthOf("AG-000").value() == 0);
    CHECK(h.depthOf("AG-001").value() == 1);
    CHECK(h.depthOf("AG-003").value() == 2);
    CHECK(!h.depthOf("AG-404").has_value());
}

void testTraversals()
{
    std::cout << "- recorridos DFS y BFS\n";
    GeographicHierarchy h;

    CHECK(h.addAgency(agency("AG-000", "Centro"), "") == OperationResult::OK);
    CHECK(h.addAgency(agency("AG-001", "Norte"), "AG-000") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-002", "Sur"), "AG-000") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-003", "Acopio Norte"), "AG-001") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-004", "Acopio Norte 2"), "AG-001") ==
          OperationResult::OK);
    CHECK(h.addAgency(agency("AG-005", "Acopio Sur"), "AG-002") ==
          OperationResult::OK);

    // Preorden: AG-001, AG-003, AG-004, AG-002, AG-005.
    auto dfs = h.descendants("AG-000");
    CHECK(sameIds(dfs, {"AG-001", "AG-003", "AG-004", "AG-002", "AG-005"}));
    CHECK(dfs.size() == 5);

    // BFS: nivel 1 (Norte, Sur) antes que nivel 2 (acopios).
    auto bfs = h.levelOrder("AG-000");
    CHECK(bfs.size() == 6);
    CHECK(bfs[0].id == "AG-000");
    CHECK(bfs[1].id == "AG-001");
    CHECK(bfs[2].id == "AG-002");
    CHECK(bfs[3].id == "AG-003");
    CHECK(bfs[4].id == "AG-004");
    CHECK(bfs[5].id == "AG-005");

    // Subarbol: desde AG-001 solo sus dos hijos.
    CHECK(sameIds(h.descendants("AG-001"), {"AG-003", "AG-004"}));
    CHECK(sameIds(h.levelOrder("AG-003"), {"AG-003"}));

    // Consultar un ID inexistente no rompe: devuelve vacio.
    CHECK(h.descendants("AG-404").is_empty());
    CHECK(h.levelOrder("AG-404").is_empty());
}

void testLargeChain()
{
    std::cout << "- cadena de 1000 agencias\n";
    GeographicHierarchy h;

    constexpr int N = 1000;

    CHECK(h.addAgency(agency("AG-10000", "Nodo 0"), "") ==
          OperationResult::OK);

    for (int i = 1; i < N; ++i)
    {
        const std::string id = "AG-" + std::to_string(10000 + i);
        const std::string parent = "AG-" + std::to_string(10000 + i - 1);

        if (h.addAgency(agency(id, "Nodo " + std::to_string(i)), parent) !=
            OperationResult::OK)
        {
            CHECK(false);
            break;
        }
    }

    CHECK(h.size() == static_cast<std::size_t>(N));
    CHECK(h.depthOf("AG-10999").value() == static_cast<std::size_t>(N - 1));
    CHECK(h.descendants("AG-10000").size() ==
          static_cast<std::size_t>(N - 1));
    CHECK(h.levelOrder("AG-10000").size() == static_cast<std::size_t>(N));
}

void testClear()
{
    std::cout << "- clear y reutilizacion\n";
    GeographicHierarchy h;

    CHECK(h.addAgency(agency("AG-000", "Centro"), "") == OperationResult::OK);
    CHECK(h.addAgency(agency("AG-001", "Norte"), "AG-000") ==
          OperationResult::OK);

    h.clear();

    CHECK(h.size() == 0);
    CHECK(!h.root().has_value());
    CHECK(!h.contains("AG-000"));
    CHECK(!h.contains("AG-001"));

    // La jerarquia puede volver a usarse tras clear().
    CHECK(h.addAgency(agency("AG-100", "Nueva raiz"), "") ==
          OperationResult::OK);
    CHECK(h.size() == 1);
    CHECK(h.root()->id == "AG-100");
}

} // namespace

int main()
{
    std::cout << "NodeDispatch - Pruebas GeographicHierarchy (S2.1)\n";

    testEmptyHierarchy();
    testInsertionAndValidation();
    testNavigation();
    testTraversals();
    testLargeChain();
    testClear();

    std::cout << "\n";

    if (failures == 0)
    {
        std::cout << "OK: GeographicHierarchy cumple todos los casos.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "FALLO: " << failures << " comprobacion(es) fallida(s).\n";
    return EXIT_FAILURE;
}
