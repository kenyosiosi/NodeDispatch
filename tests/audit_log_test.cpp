// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas de AuditLog (S2.10)
//
// Cubre el alta, el orden de insercion, el filtrado por paquete y el vaciado.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/audit_log.hpp"

using namespace nodedispatch;

namespace {

PackageEvent makeEvent(const std::string& code, PackageStatus status, std::int64_t at,
                       const std::string& note = "")
{
    PackageEvent event;
    event.trackingCode = code;
    event.status = status;
    event.at = at;
    event.note = note;
    return event;
}

} // namespace

TEST_SUITE("dominio/audit_log") {

TEST_CASE("AuditLog registra eventos en orden de llegada (S2.10)")
{
    AuditLog log;

    CHECK(log.size() == 0);
    CHECK(log.append(makeEvent("ND-1", PackageStatus::REGISTERED, 100, "alta")) == OperationResult::OK);
    CHECK(log.append(makeEvent("ND-1", PackageStatus::IN_TRANSIT, 200)) == OperationResult::OK);
    CHECK(log.append(makeEvent("ND-2", PackageStatus::REGISTERED, 150)) == OperationResult::OK);
    CHECK(log.size() == 3);

    const auto history = log.history("ND-1");
    REQUIRE(history.size() == 2);
    CHECK(history[0].status == PackageStatus::REGISTERED);
    CHECK(history[0].at == 100);
    CHECK(history[0].note == "alta");
    CHECK(history[1].status == PackageStatus::IN_TRANSIT);
    CHECK(history[1].at == 200);
}

TEST_CASE("history y count filtran por codigo (S2.10)")
{
    AuditLog log;
    log.append(makeEvent("ND-1", PackageStatus::REGISTERED, 1));
    log.append(makeEvent("ND-2", PackageStatus::REGISTERED, 2));
    log.append(makeEvent("ND-1", PackageStatus::AT_LOCAL_AGENCY, 3));

    CHECK(log.count("ND-1") == 2);
    CHECK(log.count("ND-2") == 1);
    CHECK(log.count("ND-404") == 0);

    const auto other = log.history("ND-404");
    CHECK(other.is_empty());

    const auto vacio = log.history("");
    CHECK(vacio.is_empty());
}

TEST_CASE("AuditLog rechaza eventos sin codigo (S2.10)")
{
    AuditLog log;

    CHECK(log.append(makeEvent("", PackageStatus::REGISTERED, 1)) == OperationResult::INVALID_INPUT);
    CHECK(log.size() == 0);
}

TEST_CASE("AuditLog se vacia y queda reutilizable (S2.10)")
{
    AuditLog log;
    log.append(makeEvent("ND-1", PackageStatus::REGISTERED, 1));
    log.append(makeEvent("ND-2", PackageStatus::REGISTERED, 2));

    log.clear();
    CHECK(log.size() == 0);
    CHECK(log.count("ND-1") == 0);

    CHECK(log.append(makeEvent("ND-3", PackageStatus::REGISTERED, 3)) == OperationResult::OK);
    CHECK(log.size() == 1);
}

TEST_CASE("AuditLog sostiene mil eventos (S2.10)")
{
    AuditLog log;

    for (int i = 0; i < 1000; ++i)
    {
        const std::string code = "ND-" + std::to_string(i % 10);
        REQUIRE(log.append(makeEvent(code, PackageStatus::IN_TRANSIT, i)) == OperationResult::OK);
    }

    CHECK(log.size() == 1000);
    CHECK(log.count("ND-0") == 100);
    CHECK(log.count("ND-9") == 100);

    // Dentro del mismo codigo, se conserva el orden de llegada.
    const auto history = log.history("ND-0");
    REQUIRE(history.size() == 100);
    for (std::size_t i = 1; i < history.size(); ++i)
    {
        CHECK(history[i - 1].at < history[i].at);
    }
}

} // TEST_SUITE
