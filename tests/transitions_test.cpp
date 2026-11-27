// -----------------------------------------------------------------------------
// NodeDispatch - Maquina de estados de PackageStatus (S3.3)
//
// Fija las transiciones del SDD 5.1 y las dos condiciones del 5.2 en
// canTransition(), que es la funcion que usa updateStatus. Las nueve validas
// incluyen las ocho filas de la tabla mas el caso de entrega con Cobro Destino
// ya pagado, que la regla 5.2 trata aparte.
// -----------------------------------------------------------------------------

#include <doctest/doctest.h>

#include "nodedispatch/tracking_engine.hpp"

using namespace nodedispatch;

TEST_SUITE("dominio/transiciones") {

TEST_CASE("las nueve transiciones validas se aceptan (S3.3)")
{
    const PaymentStatus paid = PaymentStatus::PAID;
    const PaymentStatus pending = PaymentStatus::PENDING;

    // 1. REGISTERED -> IN_TRANSIT
    CHECK(canTransition(PackageStatus::REGISTERED, PackageStatus::IN_TRANSIT,
                        pending, DeliveryMethod::AGENCY_PICKUP));
    // 2. REGISTERED -> RETURNED
    CHECK(canTransition(PackageStatus::REGISTERED, PackageStatus::RETURNED,
                        pending, DeliveryMethod::AGENCY_PICKUP));
    // 3. IN_TRANSIT -> AT_LOCAL_AGENCY
    CHECK(canTransition(PackageStatus::IN_TRANSIT, PackageStatus::AT_LOCAL_AGENCY,
                        pending, DeliveryMethod::AGENCY_PICKUP));
    // 4. AT_LOCAL_AGENCY -> OUT_FOR_DELIVERY (solo con reparto a domicilio)
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::OUT_FOR_DELIVERY,
                        pending, DeliveryMethod::HOME_DELIVERY));
    // 5. AT_LOCAL_AGENCY -> DELIVERED (retiro, prepago)
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::DELIVERED,
                        paid, DeliveryMethod::AGENCY_PICKUP));
    // 6. AT_LOCAL_AGENCY -> DELIVERED con Cobro Destino ya pagado (regla 5.2)
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::DELIVERED,
                        paid, DeliveryMethod::HOME_DELIVERY));
    // 7. OUT_FOR_DELIVERY -> DELIVERED
    CHECK(canTransition(PackageStatus::OUT_FOR_DELIVERY, PackageStatus::DELIVERED,
                        paid, DeliveryMethod::HOME_DELIVERY));
    // 8. OUT_FOR_DELIVERY -> RETURNED
    CHECK(canTransition(PackageStatus::OUT_FOR_DELIVERY, PackageStatus::RETURNED,
                        pending, DeliveryMethod::HOME_DELIVERY));
    // 9. AT_LOCAL_AGENCY -> RETURNED
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::RETURNED,
                        pending, DeliveryMethod::AGENCY_PICKUP));
}

TEST_CASE("las seis transiciones invalidas se rechazan (S3.3)")
{
    const PaymentStatus paid = PaymentStatus::PAID;

    // 1. REGISTERED no salta a AT_LOCAL_AGENCY.
    CHECK_FALSE(canTransition(PackageStatus::REGISTERED, PackageStatus::AT_LOCAL_AGENCY,
                              paid, DeliveryMethod::AGENCY_PICKUP));
    // 2. REGISTERED no salta a DELIVERED.
    CHECK_FALSE(canTransition(PackageStatus::REGISTERED, PackageStatus::DELIVERED,
                              paid, DeliveryMethod::AGENCY_PICKUP));
    // 3. REGISTERED no salta a OUT_FOR_DELIVERY.
    CHECK_FALSE(canTransition(PackageStatus::REGISTERED, PackageStatus::OUT_FOR_DELIVERY,
                              paid, DeliveryMethod::HOME_DELIVERY));
    // 4. IN_TRANSIT no entrega directamente.
    CHECK_FALSE(canTransition(PackageStatus::IN_TRANSIT, PackageStatus::DELIVERED,
                              paid, DeliveryMethod::AGENCY_PICKUP));
    // 5. DELIVERED es terminal.
    CHECK_FALSE(canTransition(PackageStatus::DELIVERED, PackageStatus::RETURNED,
                              paid, DeliveryMethod::AGENCY_PICKUP));
    // 6. RETURNED es terminal.
    CHECK_FALSE(canTransition(PackageStatus::RETURNED, PackageStatus::IN_TRANSIT,
                              paid, DeliveryMethod::AGENCY_PICKUP));
}

TEST_CASE("la condicion de domicilio solo aplica a OUT_FOR_DELIVERY (S3.3)")
{
    // Misma transicion, distinto metodo de entrega.
    CHECK_FALSE(canTransition(PackageStatus::AT_LOCAL_AGENCY,
                              PackageStatus::OUT_FOR_DELIVERY,
                              PaymentStatus::PAID, DeliveryMethod::AGENCY_PICKUP));
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::OUT_FOR_DELIVERY,
                        PaymentStatus::PAID, DeliveryMethod::HOME_DELIVERY));

    // El retiro en agencia no exige domicilio para entregarse.
    CHECK(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::DELIVERED,
                        PaymentStatus::PAID, DeliveryMethod::AGENCY_PICKUP));
}

TEST_CASE("la regla COD muerde en la entrega (S3.3)")
{
    // Mismo estado y metodo, distinto pago: es la regla del SDD 5.2.
    CHECK_FALSE(canTransition(PackageStatus::AT_LOCAL_AGENCY, PackageStatus::DELIVERED,
                              PaymentStatus::PENDING, DeliveryMethod::AGENCY_PICKUP));
    CHECK_FALSE(canTransition(PackageStatus::OUT_FOR_DELIVERY, PackageStatus::DELIVERED,
                              PaymentStatus::FAILED, DeliveryMethod::HOME_DELIVERY));
    CHECK(canTransition(PackageStatus::OUT_FOR_DELIVERY, PackageStatus::DELIVERED,
                        PaymentStatus::PAID, DeliveryMethod::HOME_DELIVERY));
}

TEST_CASE("updateStatus usa exactamente la misma regla (S3.3)")
{
    TrackingEngine engine;

    Package package;
    package.trackingCode = "ND-1";
    package.targetAgencyId = "AG-CCS";
    package.paymentType = PaymentType::CASH_ON_DELIVERY;
    package.paymentStatus = PaymentStatus::PENDING;
    package.deliveryMethod = DeliveryMethod::AGENCY_PICKUP;
    package.status = PackageStatus::AT_LOCAL_AGENCY;
    REQUIRE(engine.add(package) == OperationResult::OK);

    // La misma combinacion que canTransition rechaza, aqui tambien.
    CHECK(engine.updateStatus("ND-1", PackageStatus::DELIVERED) ==
          OperationResult::INVALID_STATE);

    REQUIRE(engine.updatePaymentStatus("ND-1", PaymentStatus::PAID) == OperationResult::OK);
    CHECK(engine.updateStatus("ND-1", PackageStatus::DELIVERED) == OperationResult::OK);
}

} // TEST_SUITE
