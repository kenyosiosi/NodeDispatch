// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas del generador de codigos (N2)
//
// Fija el formato ND-AAAAMMDD-NNNNNN, el reinicio diario del contador, el tope
// de 999999 y el rechazo de marcas negativas. La fecha de referencia
// (1795000000 = 2026-11-18 UTC) se obtuvo con la
// biblioteca estandar de Python, no con el codigo bajo prueba.
// -----------------------------------------------------------------------------

#include <doctest/doctest.h>

#include "nodedispatch/tracking_code.hpp"

using namespace nodedispatch;

namespace {

constexpr std::int64_t kEpoch = 0;
constexpr std::int64_t kReference = 1795000000; // 2026-11-18 UTC
constexpr std::int64_t kNextDay = kReference + 86400;

} // namespace

TEST_SUITE("dominio/tracking_code") {

TEST_CASE("formatTrackingCode fija el formato ND-AAAAMMDD-NNNNNN (N2)")
{
    CHECK(formatTrackingCode(kEpoch, 0) == "ND-19700101-000000");
    CHECK(formatTrackingCode(kEpoch, 1) == "ND-19700101-000001");
    CHECK(formatTrackingCode(kEpoch, 999999) == "ND-19700101-999999");
    CHECK(formatTrackingCode(kReference, 7) == "ND-20261118-000007");
    // Ultimo segundo del mismo dia UTC (kReference cae a media manana, asi
    // que sumarle 86399 cruzaria al dia siguiente).
    constexpr std::int64_t kLastSecond = kReference - (kReference % 86400) + 86399;
    CHECK(formatTrackingCode(kLastSecond, 7) == "ND-20261118-000007");
    CHECK(formatTrackingCode(kNextDay, 7) == "ND-20261119-000007");

    // Longitud fija: 3 + 8 + 1 + 6.
    CHECK(formatTrackingCode(kReference, 42).size() == 18);

    // Entradas fuera de rango: cadena vacia, nunca un codigo a medias.
    CHECK(formatTrackingCode(-1, 0).empty());
    CHECK(formatTrackingCode(kReference, kMaxTrackingSequence + 1).empty());
}

TEST_CASE("el generador emite codigos consecutivos y los reinicia cada dia (N2)")
{
    TrackingCodeGenerator generator;
    std::string code;

    REQUIRE(generator.next(kReference, code) == OperationResult::OK);
    CHECK(code == "ND-20261118-000001");
    REQUIRE(generator.next(kReference, code) == OperationResult::OK);
    CHECK(code == "ND-20261118-000002");
    CHECK(generator.sequence() == 2);

    // El mismo dia, un segundo despues: sigue el contador.
    REQUIRE(generator.next(kReference + 59, code) == OperationResult::OK);
    CHECK(code == "ND-20261118-000003");

    // Dia nuevo: vuelve a 000001.
    REQUIRE(generator.next(kNextDay, code) == OperationResult::OK);
    CHECK(code == "ND-20261119-000001");
    CHECK(generator.sequence() == 1);
}

TEST_CASE("el generador rechaza marcas negativas y el dia agotado (N2)")
{
    TrackingCodeGenerator generator;
    std::string code;

    CHECK(generator.next(-1, code) == OperationResult::INVALID_INPUT);
    CHECK(code.empty());
    CHECK(generator.sequence() == 0);

    // Agota el dia emitiendo los 999999 codigos: es la unica forma de comprobar
    // el borde real, y de paso verifica que el contador no se desborda.
    bool allOk = true;
    for (std::uint64_t i = 0; i < kMaxTrackingSequence; ++i)
    {
        if (generator.next(kReference, code) != OperationResult::OK)
        {
            allOk = false;
            break;
        }
    }
    CHECK(allOk);
    CHECK(generator.sequence() == kMaxTrackingSequence);
    CHECK(code == "ND-20261118-999999");

    // El siguiente ya no cabe en seis digitos: falla en vez de repetir.
    CHECK(generator.next(kReference, code) == OperationResult::INVALID_STATE);

    // Un dia nuevo vuelve a emitir.
    REQUIRE(generator.next(kNextDay, code) == OperationResult::OK);
    CHECK(code == "ND-20261119-000001");
}

TEST_CASE("mil codigos del mismo dia son unicos (N2)")
{
    TrackingCodeGenerator generator;
    std::string code;
    std::string previous;

    for (int i = 0; i < 1000; ++i)
    {
        REQUIRE(generator.next(kReference, code) == OperationResult::OK);
        CHECK(code != previous);
        previous = code;
    }

    CHECK(generator.sequence() == 1000);
    CHECK(previous == "ND-20261118-001000");
}

} // TEST_SUITE
