// -----------------------------------------------------------------------------
// NodeDispatch - S2.9 Comparador de despacho y su uso en DynHeap
//
// Fija dos cosas a la vez:
//   1. Los valores 1/2/3 de ServiceLevel, con static_assert: son un invariante
//      del comparador, no un detalle del enum.
//   2. El orden real que ve la cola de despacho, probado sobre
//      Designar::DynHeap y no solo llamando al comparador a mano.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>
#include <heap.hpp>

#include "nodedispatch/dispatch_cmp.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

using namespace nodedispatch;

namespace {

// --- Invariante de ServiceLevel (SDD 4.1 y 5.3) -----------------------------
static_assert(static_cast<int>(ServiceLevel::EXPRESS_24H) == 1,
              "EXPRESS_24H debe valer 1: DispatchCmp ordena de menor a mayor");
static_assert(static_cast<int>(ServiceLevel::PRIORITY) == 2,
              "PRIORITY debe valer 2");
static_assert(static_cast<int>(ServiceLevel::STANDARD) == 3,
              "STANDARD debe valer 3");

using Queue = Designar::DynHeap<DispatchEntry, DispatchCmp>;

DispatchEntry entry(const std::string& code, ServiceLevel level, std::int64_t at)
{
    DispatchEntry e;
    e.trackingCode = code;
    e.agencyId = "AG-CCS";
    e.level = level;
    e.createdAt = at;
    return e;
}

} // namespace

TEST_CASE("DispatchCmp ordena por nivel de servicio y luego por llegada")
{
    const DispatchCmp cmp;

    const DispatchEntry express = entry("ND-EXPRESS", ServiceLevel::EXPRESS_24H, 500);
    const DispatchEntry priority = entry("ND-PRIORITY", ServiceLevel::PRIORITY, 100);
    const DispatchEntry standard = entry("ND-STANDARD", ServiceLevel::STANDARD, 50);

    // El nivel pesa mas que la antiguedad: un EXPRESS nuevo sale antes que un
    // STANDARD viejo.
    CHECK(cmp(express, standard));
    CHECK(cmp(express, priority));
    CHECK(cmp(priority, standard));
    CHECK_FALSE(cmp(standard, express));

    // Mismo nivel: gana el que llego antes.
    const DispatchEntry priorityOld = entry("ND-OLD", ServiceLevel::PRIORITY, 10);
    CHECK(cmp(priorityOld, priority));
    CHECK_FALSE(cmp(priority, priorityOld));

    // Un elemento nunca es anterior a si mismo.
    CHECK_FALSE(cmp(express, express));
}

TEST_CASE("DispatchCmp desempata por codigo cuando nivel y hora coinciden")
{
    const DispatchCmp cmp;

    const DispatchEntry a = entry("ND-AAA", ServiceLevel::STANDARD, 42);
    const DispatchEntry b = entry("ND-BBB", ServiceLevel::STANDARD, 42);

    CHECK(cmp(a, b));
    CHECK_FALSE(cmp(b, a));

    // El orden es total: con el desempate por codigo, el heap es reproducible
    // sin importar el orden de insercion.
    CHECK(cmp(a, b) != cmp(b, a));
}

TEST_CASE("DynHeap con DispatchCmp entrega primero el paquete mas urgente")
{
    Queue queue;

    // Se insertan a proposito en el peor orden posible.
    queue.insert(entry("ND-STANDARD-2", ServiceLevel::STANDARD, 900));
    queue.insert(entry("ND-PRIORITY-2", ServiceLevel::PRIORITY, 800));
    queue.insert(entry("ND-STANDARD-1", ServiceLevel::STANDARD, 100));
    queue.insert(entry("ND-EXPRESS-2", ServiceLevel::EXPRESS_24H, 700));
    queue.insert(entry("ND-PRIORITY-1", ServiceLevel::PRIORITY, 200));
    queue.insert(entry("ND-EXPRESS-1", ServiceLevel::EXPRESS_24H, 300));

    REQUIRE(queue.size() == 6);
    CHECK(queue.top().trackingCode == "ND-EXPRESS-1");

    // Orden de salida esperado: EXPRESS por antiguedad, luego PRIORITY, luego
    // STANDARD. Dentro de cada nivel, el de createdAt menor primero.
    const std::string expected[] = {
        "ND-EXPRESS-1", "ND-EXPRESS-2", "ND-PRIORITY-1",
        "ND-PRIORITY-2", "ND-STANDARD-1", "ND-STANDARD-2",
    };

    for (const std::string& code : expected)
    {
        REQUIRE_FALSE(queue.is_empty());
        CHECK(queue.top().trackingCode == code);
        CHECK(queue.get().trackingCode == code);
    }

    CHECK(queue.is_empty());
}

TEST_CASE("DynHeap con DispatchCmp queda vacia al extraer todo")
{
    Queue queue;
    CHECK(queue.is_empty());

    queue.insert(entry("ND-1", ServiceLevel::PRIORITY, 1));
    queue.insert(entry("ND-2", ServiceLevel::EXPRESS_24H, 2));

    CHECK(queue.get().trackingCode == "ND-2");
    CHECK(queue.get().trackingCode == "ND-1");
    CHECK(queue.is_empty());
}
