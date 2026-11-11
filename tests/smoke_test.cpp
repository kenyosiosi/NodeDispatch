// -----------------------------------------------------------------------------
// NodeDispatch - S1.0 Smoke test de DeSiGNAR v4.0.1
//
// Verifica en un solo ejecutable que las cinco estructuras que usara el nucleo
// compilan y se comportan como espera el SDD:
//
//   1) Designar::MTreeNode<Agency>          -> jerarquia N-aria
//   2) Designar::TreeMap<..., RbTree>       -> inventario ordenado (RB)
//   3) Designar::HashMap<string, Package>   -> rastreo global
//   4) Designar::DynHeap<DispatchEntry>     -> cola de prioridad (min-heap)
//   5) Designar::DynArray<int>              -> reportes / resultados
//
// Compilar:  g++ -std=c++17 -I include ... (ver CMakeLists.txt)
// -----------------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#include <array.hpp>
#include <heap.hpp>
#include <map.hpp>
#include <mtreenode.hpp>
#include <rbtree.hpp>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/package.hpp"

using namespace Designar;
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

void section(const char* name)
{
    std::cout << "- " << name << "\n";
}

Agency makeAgency(const std::string& id, const std::string& name,
                  const std::string& parentId)
{
    Agency a;
    a.id = id;
    a.name = name;
    a.location = Address{"Tachira", "San Cristobal", "Calle 1", "5001"};
    a.parentId = parentId;
    return a;
}

Package makePackage(const std::string& code, ServiceLevel level)
{
    Package p;
    p.trackingCode = code;
    p.senderId = "V-1001";
    p.receiverId = "V-2002";
    p.targetAgencyId = "AG-001";
    p.weightKg = 1.5;
    p.baseCents = 500;
    p.deliverySurchargeCents = 200;
    p.paymentType = PaymentType::CASH_ON_DELIVERY;
    p.paymentStatus = PaymentStatus::PENDING;
    p.deliveryMethod = DeliveryMethod::HOME_DELIVERY;
    p.serviceLevel = level;
    p.status = PackageStatus::AT_LOCAL_AGENCY;
    p.createdAt = 100;
    p.updatedAt = 100;
    return p;
}

// En un min-heap, "menor" debe significar "mas urgente".
struct DispatchCmp {
    bool operator()(const DispatchEntry& a, const DispatchEntry& b) const
    {
        if (a.level != b.level)
            return static_cast<int>(a.level) < static_cast<int>(b.level);
        return a.createdAt < b.createdAt;
    }
};

} // namespace

int main()
{
    std::cout << "NodeDispatch - Smoke test DeSiGNAR v4.0.1 (C++17)\n";

    // -------------------------------------------------------------------------
    // 1) MTreeNode<Agency>: arbol N-ario
    // -------------------------------------------------------------------------
    section("MTreeNode<Agency> (jerarquia N-aria)");
    {
        auto* root = new MTreeNode<Agency>(makeAgency("AG-000", "Centro", ""));
        auto* c1 = new MTreeNode<Agency>(makeAgency("AG-001", "Sucursal Norte", "AG-000"));
        auto* c2 = new MTreeNode<Agency>(makeAgency("AG-002", "Sucursal Sur", "AG-000"));
        auto* g1 = new MTreeNode<Agency>(makeAgency("AG-003", "Acopio", "AG-001"));

        root->append_child(c1);
        root->append_child(c2);
        c1->append_child(g1);

        CHECK(root->get_key().id == "AG-000");
        CHECK(c1->get_parent() == root);
        CHECK(root->get_first_child() == c1);
        CHECK(root->get_last_child() == c2);

        int visited = 0;
        std::function<void(MTreeNode<Agency>*)> dfs =
            [&](MTreeNode<Agency>* node) {
                ++visited;
                node->for_each_child([&](MTreeNode<Agency>* child) { dfs(child); });
            };
        dfs(root);
        CHECK(visited == 4);

        int direct_children = 0;
        root->for_each_child(
            [&](MTreeNode<Agency>* child) {
                (void)child;
                ++direct_children;
            });
        CHECK(direct_children == 2);

        MTreeNode<Agency>::destroy_tree(root);
        CHECK(root == nullptr);
    }

    // -------------------------------------------------------------------------
    // 2) TreeMap con backend RbTree: inventario ordenado
    // -------------------------------------------------------------------------
    section("TreeMap<..., RbTree> (inventario ordenado)");
    {
        TreeMap<std::string, Package, std::less<std::string>, RbTree> inventory;

        const Package p1 = makePackage("TRK-001", ServiceLevel::STANDARD);
        const Package p2 = makePackage("TRK-002", ServiceLevel::EXPRESS_24H);

        CHECK(inventory.insert(p2.trackingCode, p2) != nullptr);
        CHECK(inventory.insert(p1.trackingCode, p1) != nullptr);
        CHECK(inventory.size() == 2);

        // Clave duplicada: DeSiGNAR devuelve nullptr.
        CHECK(inventory.insert(p1.trackingCode, p1) == nullptr);

        CHECK(inventory.search("TRK-001") != nullptr);
        CHECK(inventory.search("TRK-001")->serviceLevel == ServiceLevel::STANDARD);
        CHECK(inventory.search("TRK-404") == nullptr);

        CHECK(inventory.remove("TRK-002"));
        CHECK(inventory.search("TRK-002") == nullptr);
        CHECK(inventory.size() == 1);
    }

    // -------------------------------------------------------------------------
    // 3) HashMap<string, Package>: rastreo global
    // -------------------------------------------------------------------------
    section("HashMap<string, Package> (rastreo global)");
    {
        HashMap<std::string, Package> tracking;

        const Package p = makePackage("TRK-100", ServiceLevel::PRIORITY);
        CHECK(tracking.insert(p.trackingCode, p) != nullptr);
        CHECK(tracking.insert(p.trackingCode, p) == nullptr); // duplicado

        Package* found = tracking.search("TRK-100");
        CHECK(found != nullptr);
        CHECK(found->receiverId == "V-2002");
        CHECK(tracking.search("TRK-999") == nullptr);

        // RNF-02: 100 000 inserciones y busquedas sin degradar.
        HashMap<std::string, int> bulk;
        constexpr int N = 100000;
        for (int i = 0; i < N; ++i)
            bulk.insert("CODE-" + std::to_string(i), i);

        CHECK(bulk.size() == static_cast<nat_t>(N));
        CHECK(bulk.search("CODE-0") != nullptr);
        CHECK(bulk.search("CODE-99999") != nullptr);
        CHECK(bulk.search("CODE-100000") == nullptr);
    }

    // -------------------------------------------------------------------------
    // 4) DynHeap<DispatchEntry, DispatchCmp>: cola de prioridad (min-heap)
    // -------------------------------------------------------------------------
    section("DynHeap<DispatchEntry> (cola de prioridad)");
    {
        DynHeap<DispatchEntry, DispatchCmp> heap;

        const DispatchEntry standard{"TRK-A", "AG-001", ServiceLevel::STANDARD, 10};
        const DispatchEntry express{"TRK-B", "AG-001", ServiceLevel::EXPRESS_24H, 30};
        const DispatchEntry priority{"TRK-C", "AG-001", ServiceLevel::PRIORITY, 20};
        const DispatchEntry express_old{"TRK-D", "AG-001", ServiceLevel::EXPRESS_24H, 5};

        heap.insert(standard);
        heap.insert(express);
        heap.insert(priority);
        heap.insert(express_old);

        CHECK(heap.size() == 4);
        CHECK(!heap.is_empty());
        CHECK(heap.top().trackingCode == "TRK-D"); // Express mas antiguo

        CHECK(heap.get().trackingCode == "TRK-D");
        CHECK(heap.get().trackingCode == "TRK-B");
        CHECK(heap.get().trackingCode == "TRK-C");
        CHECK(heap.get().trackingCode == "TRK-A");
        CHECK(heap.is_empty());
    }

    // -------------------------------------------------------------------------
    // 5) DynArray<int>: reportes y resultados
    // -------------------------------------------------------------------------
    section("DynArray<int> (reportes)");
    {
        DynArray<int> values;
        for (int i = 0; i < 10; ++i)
            values.append(i);

        CHECK(values.size() == 10);
        CHECK(values[0] == 0);
        CHECK(values[9] == 9);

        int sum = 0;
        for (nat_t i = 0; i < values.size(); ++i)
            sum += values[i];
        CHECK(sum == 45);
    }

    // -------------------------------------------------------------------------
    std::cout << "\n";
    if (failures == 0)
    {
        std::cout << "OK: las 5 estructuras de DeSiGNAR funcionan como se espera.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "FALLO: " << failures << " comprobacion(es) fallida(s).\n";
    return EXIT_FAILURE;
}
