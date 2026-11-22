// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas de PriorityDispatchQueue (S2.8)
//
// Cubre el orden de despacho (nivel, FIFO y desempate estable), la eliminacion
// perezosa por estado, agencia, nivel o marca de admision, y el mantenimiento
// de las colas.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/dispatch_queue.hpp"
#include "nodedispatch/tracking_engine.hpp"

using namespace nodedispatch;

namespace {

constexpr std::int64_t kBase = 1795000000;

Package makePackage(const std::string& code,
                    ServiceLevel level = ServiceLevel::STANDARD,
                    std::int64_t createdAt = kBase,
                    PackageStatus status = PackageStatus::AT_LOCAL_AGENCY,
                    const std::string& agency = "AG-CCS")
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = agency;
    package.weightKg = 1.0;
    package.baseCents = 1000;
    package.paymentType = PaymentType::PREPAID;
    package.paymentStatus = PaymentStatus::PAID;
    package.deliveryMethod = DeliveryMethod::AGENCY_PICKUP;
    package.serviceLevel = level;
    package.status = status;
    package.createdAt = createdAt;
    package.updatedAt = createdAt;
    return package;
}

DispatchEntry entryFor(const Package& package)
{
    DispatchEntry entry;
    entry.trackingCode = package.trackingCode;
    entry.agencyId = package.targetAgencyId;
    entry.level = package.serviceLevel;
    entry.createdAt = package.createdAt;
    return entry;
}

} // namespace

TEST_SUITE("dominio/dispatch_queue") {

TEST_CASE("despacha por nivel de servicio (S2.7)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    const Package normal = makePackage("ND-N", ServiceLevel::STANDARD, kBase);
    const Package urgente = makePackage("ND-E", ServiceLevel::EXPRESS_24H, kBase + 100);
    const Package medio = makePackage("ND-P", ServiceLevel::PRIORITY, kBase + 200);

    for (const Package& p : {normal, urgente, medio})
    {
        REQUIRE(engine.add(p) == OperationResult::OK);
        REQUIRE(queue.enqueue(entryFor(p)) == OperationResult::OK);
    }

    auto first = queue.dispatch("AG-CCS", engine);
    REQUIRE(first.has_value());
    CHECK(first->trackingCode == "ND-E");

    auto second = queue.dispatch("AG-CCS", engine);
    REQUIRE(second.has_value());
    CHECK(second->trackingCode == "ND-P");

    auto third = queue.dispatch("AG-CCS", engine);
    REQUIRE(third.has_value());
    CHECK(third->trackingCode == "ND-N");

    CHECK_FALSE(queue.dispatch("AG-CCS", engine).has_value());
}

TEST_CASE("desempata por llegada y por codigo (S2.8)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    const Package tarde = makePackage("ND-B", ServiceLevel::STANDARD, kBase + 10);
    const Package temprano = makePackage("ND-C", ServiceLevel::STANDARD, kBase);
    const Package empate1 = makePackage("ND-A", ServiceLevel::STANDARD, kBase + 5);
    const Package empate2 = makePackage("ND-D", ServiceLevel::STANDARD, kBase + 5);

    for (const Package& p : {tarde, temprano, empate1, empate2})
    {
        REQUIRE(engine.add(p) == OperationResult::OK);
        REQUIRE(queue.enqueue(entryFor(p)) == OperationResult::OK);
    }

    CHECK(queue.dispatch("AG-CCS", engine)->trackingCode == "ND-C"); // mas viejo
    CHECK(queue.dispatch("AG-CCS", engine)->trackingCode == "ND-A"); // empate: codigo menor
    CHECK(queue.dispatch("AG-CCS", engine)->trackingCode == "ND-D");
    CHECK(queue.dispatch("AG-CCS", engine)->trackingCode == "ND-B");
}

TEST_CASE("descarta entradas de paquetes que ya no estan en la agencia (S2.8)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    const Package enTransito = makePackage("ND-1", ServiceLevel::EXPRESS_24H, kBase + 2,
                                           PackageStatus::IN_TRANSIT);
    const Package entregado = makePackage("ND-2", ServiceLevel::EXPRESS_24H, kBase + 3,
                                          PackageStatus::DELIVERED);
    const Package bueno = makePackage("ND-3", ServiceLevel::STANDARD, kBase + 4);

    for (const Package& p : {enTransito, entregado, bueno})
    {
        REQUIRE(engine.add(p) == OperationResult::OK);
        REQUIRE(queue.enqueue(entryFor(p)) == OperationResult::OK);
    }

    CHECK(queue.size("AG-CCS") == 3);

    auto next = queue.dispatch("AG-CCS", engine);
    REQUIRE(next.has_value());
    CHECK(next->trackingCode == "ND-3"); // los dos urgentes eran obsoletos

    CHECK(queue.size("AG-CCS") == 0); // y quedaron descartados
    CHECK_FALSE(queue.dispatch("AG-CCS", engine).has_value());
}

TEST_CASE("descarta entradas de otra agencia o de paquetes borrados (S2.8)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    const Package otra = makePackage("ND-1", ServiceLevel::EXPRESS_24H, kBase, PackageStatus::AT_LOCAL_AGENCY, "AG-MAR");
    const Package bueno = makePackage("ND-2", ServiceLevel::STANDARD, kBase + 1);

    REQUIRE(engine.add(otra) == OperationResult::OK);
    REQUIRE(engine.add(bueno) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(otra)) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(bueno)) == OperationResult::OK);

    CHECK(queue.dispatch("AG-CCS", engine)->trackingCode == "ND-2");

    // Paquete borrado: la entrada tambien es obsoleta.
    REQUIRE(engine.remove("ND-2"));
    REQUIRE(queue.enqueue(entryFor(bueno)) == OperationResult::OK);
    CHECK_FALSE(queue.dispatch("AG-CCS", engine).has_value());
}

TEST_CASE("el cambio de nivel de servicio invalida la entrada vieja (S2.8)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    Package paquete = makePackage("ND-1", ServiceLevel::STANDARD, kBase);
    REQUIRE(engine.add(paquete) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(paquete)) == OperationResult::OK);

    // El operador sube el paquete a express: se encola la entrada nueva.
    paquete.serviceLevel = ServiceLevel::EXPRESS_24H;
    REQUIRE(engine.remove("ND-1"));
    REQUIRE(engine.add(paquete) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(paquete)) == OperationResult::OK);

    auto next = queue.dispatch("AG-CCS", engine);
    REQUIRE(next.has_value());
    CHECK(next->level == ServiceLevel::EXPRESS_24H);

    // La entrada vieja (STANDARD) se descarta: el paquete ya no es STANDARD.
    CHECK_FALSE(queue.dispatch("AG-CCS", engine).has_value());
}

TEST_CASE("colas independientes por agencia (S2.8)")
{
    TrackingEngine engine;
    PriorityDispatchQueue queue;

    const Package a = makePackage("ND-A", ServiceLevel::STANDARD, kBase, PackageStatus::AT_LOCAL_AGENCY, "AG-CCS");
    const Package b = makePackage("ND-B", ServiceLevel::STANDARD, kBase, PackageStatus::AT_LOCAL_AGENCY, "AG-MAR");

    REQUIRE(engine.add(a) == OperationResult::OK);
    REQUIRE(engine.add(b) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(a)) == OperationResult::OK);
    REQUIRE(queue.enqueue(entryFor(b)) == OperationResult::OK);

    CHECK(queue.agencyCount() == 2);
    CHECK(queue.size("AG-CCS") == 1);
    CHECK(queue.size("AG-MAR") == 1);
    CHECK(queue.size("AG-404") == 0);
    CHECK(queue.is_empty("AG-404"));

    CHECK(queue.dispatch("AG-MAR", engine)->trackingCode == "ND-B");
    CHECK(queue.is_empty("AG-MAR"));
    CHECK_FALSE(queue.is_empty("AG-CCS"));

    queue.clear();
    CHECK(queue.agencyCount() == 0);
    CHECK(queue.size("AG-CCS") == 0);
}

TEST_CASE("enqueue valida codigo y agencia (S2.7)")
{
    PriorityDispatchQueue queue;

    DispatchEntry vacio;
    CHECK(queue.enqueue(vacio) == OperationResult::INVALID_INPUT);

    vacio.trackingCode = "ND-1";
    CHECK(queue.enqueue(vacio) == OperationResult::INVALID_INPUT); // sin agencia

    vacio.agencyId = "AG-CCS";
    CHECK(queue.enqueue(vacio) == OperationResult::OK);
    CHECK(queue.agencyCount() == 1);
}

} // TEST_SUITE
