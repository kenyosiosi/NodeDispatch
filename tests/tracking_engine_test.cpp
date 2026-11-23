// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas de TrackingEngine (S2.6)
//
// Cubre el alta y la busqueda, las ocho transiciones permitidas del SDD 5.1,
// las prohibidas, la regla de entrega del 5.2 y la copia segura.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/tracking_engine.hpp"

using namespace nodedispatch;

namespace {

Package makePackage(const std::string& code,
                    DeliveryMethod method = DeliveryMethod::AGENCY_PICKUP,
                    PaymentType payment = PaymentType::PREPAID,
                    PaymentStatus paymentStatus = PaymentStatus::PAID)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = "AG-CCS";
    package.weightKg = 1.5;
    package.baseCents = 1000;
    package.deliverySurchargeCents = 200;
    package.paymentType = payment;
    package.paymentStatus = paymentStatus;
    package.deliveryMethod = method;
    package.serviceLevel = ServiceLevel::STANDARD;
    package.status = PackageStatus::REGISTERED;
    package.createdAt = 1795000000;
    package.updatedAt = 1795000000;
    return package;
}

// Lleva un paquete hasta el estado pedido por la ruta valida mas corta.
void driveTo(TrackingEngine& engine, const std::string& code, PackageStatus target)
{
    switch (target)
    {
    case PackageStatus::REGISTERED:
        return;
    case PackageStatus::IN_TRANSIT:
        REQUIRE(engine.updateStatus(code, PackageStatus::IN_TRANSIT) == OperationResult::OK);
        return;
    case PackageStatus::AT_LOCAL_AGENCY:
        driveTo(engine, code, PackageStatus::IN_TRANSIT);
        REQUIRE(engine.updateStatus(code, PackageStatus::AT_LOCAL_AGENCY) == OperationResult::OK);
        return;
    case PackageStatus::OUT_FOR_DELIVERY:
        driveTo(engine, code, PackageStatus::AT_LOCAL_AGENCY);
        REQUIRE(engine.updateStatus(code, PackageStatus::OUT_FOR_DELIVERY) == OperationResult::OK);
        return;
    default:
        FAIL("driveTo no cubre ese estado");
    }
}

} // namespace

TEST_SUITE("dominio/tracking_engine") {

TEST_CASE("TrackingEngine da de alta y devuelve copias (S2.5)")
{
    TrackingEngine engine;

    CHECK(engine.size() == 0);
    CHECK(engine.add(makePackage("ND-20261118-000001")) == OperationResult::OK);
    CHECK(engine.size() == 1);
    CHECK(engine.contains("ND-20261118-000001"));

    const auto found = engine.find("ND-20261118-000001");
    REQUIRE(found.has_value());
    CHECK(found->targetAgencyId == "AG-CCS");

    // La copia no toca el original.
    auto copy = engine.find("ND-20261118-000001");
    copy->status = PackageStatus::DELIVERED;
    CHECK(engine.find("ND-20261118-000001")->status == PackageStatus::REGISTERED);

    CHECK_FALSE(engine.find("ND-404").has_value());
}

TEST_CASE("TrackingEngine rechaza codigo vacio y duplicado (S2.5)")
{
    TrackingEngine engine;

    CHECK(engine.add(makePackage("")) == OperationResult::INVALID_INPUT);
    CHECK(engine.size() == 0);

    CHECK(engine.add(makePackage("ND-1")) == OperationResult::OK);
    CHECK(engine.add(makePackage("ND-1")) == OperationResult::DUPLICATE);
    CHECK(engine.size() == 1);
    CHECK_FALSE(engine.contains(""));
}

TEST_CASE("la tabla 5.1 deja pasar las ocho transiciones validas (S2.6)")
{
    TrackingEngine engine;
    // OUT_FOR_DELIVERY exige HOME_DELIVERY (SDD 5.1).
    engine.add(makePackage("ND-A", DeliveryMethod::HOME_DELIVERY));
    driveTo(engine, "ND-A", PackageStatus::AT_LOCAL_AGENCY);
    CHECK(engine.updateStatus("ND-A", PackageStatus::OUT_FOR_DELIVERY) == OperationResult::OK);
    CHECK(engine.updateStatus("ND-A", PackageStatus::DELIVERED) == OperationResult::OK);

    TrackingEngine pickup;
    pickup.add(makePackage("ND-B"));
    driveTo(pickup, "ND-B", PackageStatus::AT_LOCAL_AGENCY);
    CHECK(pickup.updateStatus("ND-B", PackageStatus::DELIVERED) == OperationResult::OK);

    TrackingEngine cancel;
    cancel.add(makePackage("ND-C"));
    CHECK(cancel.updateStatus("ND-C", PackageStatus::RETURNED) == OperationResult::OK);

    TrackingEngine absent;
    absent.add(makePackage("ND-D", DeliveryMethod::HOME_DELIVERY));
    driveTo(absent, "ND-D", PackageStatus::OUT_FOR_DELIVERY);
    CHECK(absent.updateStatus("ND-D", PackageStatus::RETURNED) == OperationResult::OK);

    // AT_LOCAL_AGENCY -> RETURNED (pago fallido)
    TrackingEngine failed;
    failed.add(makePackage("ND-E"));
    driveTo(failed, "ND-E", PackageStatus::AT_LOCAL_AGENCY);
    CHECK(failed.updateStatus("ND-E", PackageStatus::RETURNED) == OperationResult::OK);
}

TEST_CASE("las transiciones fuera de la tabla se rechazan (S2.6)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-1"));

    // REGISTERED solo sale a IN_TRANSIT o RETURNED.
    CHECK(engine.updateStatus("ND-1", PackageStatus::DELIVERED) == OperationResult::INVALID_STATE);
    CHECK(engine.updateStatus("ND-1", PackageStatus::AT_LOCAL_AGENCY) == OperationResult::INVALID_STATE);
    CHECK(engine.updateStatus("ND-1", PackageStatus::OUT_FOR_DELIVERY) == OperationResult::INVALID_STATE);
    // Repetir el estado actual no es una transicion de la tabla.
    CHECK(engine.updateStatus("ND-1", PackageStatus::REGISTERED) == OperationResult::INVALID_STATE);

    driveTo(engine, "ND-1", PackageStatus::AT_LOCAL_AGENCY);
    // IN_TRANSIT -> DELIVERED no existe.
    CHECK(engine.updateStatus("ND-1", PackageStatus::IN_TRANSIT) == OperationResult::INVALID_STATE);

    CHECK(engine.updateStatus("ND-404", PackageStatus::IN_TRANSIT) == OperationResult::NOT_FOUND);
}

TEST_CASE("OUT_FOR_DELIVERY exige HOME_DELIVERY (S2.6)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-RETIRO", DeliveryMethod::AGENCY_PICKUP));
    driveTo(engine, "ND-RETIRO", PackageStatus::AT_LOCAL_AGENCY);

    CHECK(engine.updateStatus("ND-RETIRO", PackageStatus::OUT_FOR_DELIVERY) ==
          OperationResult::INVALID_STATE);

    TrackingEngine home;
    home.add(makePackage("ND-DOMICILIO", DeliveryMethod::HOME_DELIVERY));
    driveTo(home, "ND-DOMICILIO", PackageStatus::AT_LOCAL_AGENCY);
    CHECK(home.updateStatus("ND-DOMICILIO", PackageStatus::OUT_FOR_DELIVERY) ==
          OperationResult::OK);
}

TEST_CASE("Cobro Destino no se entrega sin pagar (S2.6, regla 5.2)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-COD", DeliveryMethod::AGENCY_PICKUP,
                           PaymentType::CASH_ON_DELIVERY, PaymentStatus::PENDING));
    driveTo(engine, "ND-COD", PackageStatus::AT_LOCAL_AGENCY);

    CHECK(engine.updateStatus("ND-COD", PackageStatus::DELIVERED) ==
          OperationResult::INVALID_STATE);

    // Se cobra y ahora si se entrega.
    CHECK(engine.updatePaymentStatus("ND-COD", PaymentStatus::PAID) == OperationResult::OK);
    CHECK(engine.updateStatus("ND-COD", PackageStatus::DELIVERED) == OperationResult::OK);
}

TEST_CASE("un paquete entregado o devuelto ya no se toca (S2.6)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-1"));
    driveTo(engine, "ND-1", PackageStatus::AT_LOCAL_AGENCY);
    REQUIRE(engine.updateStatus("ND-1", PackageStatus::DELIVERED) == OperationResult::OK);

    CHECK(engine.updateStatus("ND-1", PackageStatus::RETURNED) == OperationResult::INVALID_STATE);
    CHECK(engine.updatePaymentStatus("ND-1", PaymentStatus::FAILED) ==
          OperationResult::INVALID_STATE);

    TrackingEngine returned;
    returned.add(makePackage("ND-2"));
    REQUIRE(returned.updateStatus("ND-2", PackageStatus::RETURNED) == OperationResult::OK);
    CHECK(returned.updateStatus("ND-2", PackageStatus::IN_TRANSIT) == OperationResult::INVALID_STATE);
}

TEST_CASE("el pago se actualiza mientras el paquete esta abierto (S2.6)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-1", DeliveryMethod::AGENCY_PICKUP,
                           PaymentType::CASH_ON_DELIVERY, PaymentStatus::PENDING));

    CHECK(engine.updatePaymentStatus("ND-1", PaymentStatus::FAILED) == OperationResult::OK);
    CHECK(engine.find("ND-1")->paymentStatus == PaymentStatus::FAILED);
    // Reintento: el mismo valor es idempotente.
    CHECK(engine.updatePaymentStatus("ND-1", PaymentStatus::FAILED) == OperationResult::OK);
    CHECK(engine.updatePaymentStatus("ND-1", PaymentStatus::PAID) == OperationResult::OK);
    CHECK(engine.updatePaymentStatus("ND-404", PaymentStatus::PAID) == OperationResult::NOT_FOUND);
}

TEST_CASE("TrackingEngine elimina, cuenta y filtra por estado (S2.5)")
{
    TrackingEngine engine;
    engine.add(makePackage("ND-1"));
    engine.add(makePackage("ND-2"));
    engine.add(makePackage("ND-3"));
    driveTo(engine, "ND-3", PackageStatus::AT_LOCAL_AGENCY);

    CHECK(engine.size() == 3);
    CHECK(engine.byStatus(PackageStatus::REGISTERED).size() == 2);
    CHECK(engine.byStatus(PackageStatus::AT_LOCAL_AGENCY).size() == 1);
    CHECK(engine.byStatus(PackageStatus::DELIVERED).is_empty());

    CHECK(engine.remove("ND-1"));
    CHECK_FALSE(engine.remove("ND-1"));
    CHECK(engine.size() == 2);

    engine.clear();
    CHECK(engine.size() == 0);
    CHECK(engine.add(makePackage("ND-4")) == OperationResult::OK);
}

TEST_CASE("mil paquetes se indexan y se encuentran (S2.6)")
{
    TrackingEngine engine;
    for (int i = 0; i < 1000; ++i)
    {
        REQUIRE(engine.add(makePackage("ND-20261118-" + std::to_string(100000 + i))) ==
                OperationResult::OK);
    }

    CHECK(engine.size() == 1000);
    for (int i = 0; i < 1000; i += 97)
    {
        CHECK(engine.contains("ND-20261118-" + std::to_string(100000 + i)));
    }
}

} // TEST_SUITE
