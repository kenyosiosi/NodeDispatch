// -----------------------------------------------------------------------------
// NodeDispatch - Cinco metodos de cambio de estado en la fachada (S3.4)
//
// La tarjeta pide cinco operaciones. El contrato congelado (SDD 7) expone tres
// metodos publicos que devuelven bool; los tres "mark*" son los caminos
// internos de updatePackageStatus y se prueban a traves de el. Un test por
// operacion, como pide el DoD, sin ampliar el contrato a 18 metodos.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/node_dispatch.hpp"

using namespace nodedispatch;

namespace {

Client cliente(const std::string& id)
{
    Client client;
    client.nationalId = id;
    client.firstName = "Ana";
    client.lastName = "Perez";
    return client;
}

Agency agencia(const std::string& id)
{
    Agency agency;
    agency.id = id;
    agency.name = "Caracas";
    return agency;
}

Package paquete(const std::string& code, PaymentType tipo, DeliveryMethod metodo)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = "AG-CCS";
    package.weightKg = 1.0;
    package.baseCents = 1500;
    package.deliverySurchargeCents = 300;
    package.paymentType = tipo;
    package.deliveryMethod = metodo;
    package.serviceLevel = ServiceLevel::STANDARD;
    package.status = PackageStatus::AT_LOCAL_AGENCY;
    package.createdAt = 1795000000;
    return package;
}

struct Fixture {
    NodeDispatch app;
    Fixture()
    {
        app.addAgency(agencia("AG-CCS"), "");
        app.registerClient(cliente("V-1"));
        app.registerClient(cliente("V-2"));
    }
};

} // namespace

TEST_SUITE("dominio/fachada_estados") {

TEST_CASE("1. updatePaymentStatus cambia el pago mientras el paquete esta abierto (S3.4)")
{
    Fixture f;
    REQUIRE(f.app.registerPackage(paquete("ND-1", PaymentType::CASH_ON_DELIVERY,
                                          DeliveryMethod::AGENCY_PICKUP)));

    CHECK(f.app.updatePaymentStatus("ND-1", PaymentStatus::PAID));
    CHECK(f.app.trackPackage("ND-1")->paymentStatus == PaymentStatus::PAID);

    // Sobre un paquete cerrado no se toca el pago.
    REQUIRE(f.app.updatePackageStatus("ND-1", PackageStatus::DELIVERED));
    CHECK_FALSE(f.app.updatePaymentStatus("ND-1", PaymentStatus::FAILED));

    CHECK_FALSE(f.app.updatePaymentStatus("ND-404", PaymentStatus::PAID));
}

TEST_CASE("2. markOutForDelivery: solo con reparto a domicilio (S3.4)")
{
    Fixture f;
    REQUIRE(f.app.registerPackage(paquete("ND-DOM", PaymentType::PREPAID,
                                          DeliveryMethod::HOME_DELIVERY)));
    REQUIRE(f.app.registerPackage(paquete("ND-RET", PaymentType::PREPAID,
                                          DeliveryMethod::AGENCY_PICKUP)));

    CHECK(f.app.updatePackageStatus("ND-DOM", PackageStatus::OUT_FOR_DELIVERY));
    CHECK(f.app.trackPackage("ND-DOM")->status == PackageStatus::OUT_FOR_DELIVERY);

    // El retiro en agencia no pasa por reparto.
    CHECK_FALSE(f.app.updatePackageStatus("ND-RET", PackageStatus::OUT_FOR_DELIVERY));
    CHECK(f.app.trackPackage("ND-RET")->status == PackageStatus::AT_LOCAL_AGENCY);
}

TEST_CASE("3. markDelivered: exige pago valido (S3.4)")
{
    Fixture f;
    REQUIRE(f.app.registerPackage(paquete("ND-PRE", PaymentType::PREPAID,
                                          DeliveryMethod::AGENCY_PICKUP)));
    REQUIRE(f.app.registerPackage(paquete("ND-COD", PaymentType::CASH_ON_DELIVERY,
                                          DeliveryMethod::AGENCY_PICKUP)));

    CHECK(f.app.updatePackageStatus("ND-PRE", PackageStatus::DELIVERED));
    CHECK(f.app.trackPackage("ND-PRE")->status == PackageStatus::DELIVERED);

    // El COD sin pagar no se entrega.
    CHECK_FALSE(f.app.updatePackageStatus("ND-COD", PackageStatus::DELIVERED));
    CHECK(f.app.trackPackage("ND-COD")->status == PackageStatus::AT_LOCAL_AGENCY);
}

TEST_CASE("4. markReturned: desde la agencia y desde reparto (S3.4)")
{
    Fixture f;
    REQUIRE(f.app.registerPackage(paquete("ND-1", PaymentType::CASH_ON_DELIVERY,
                                          DeliveryMethod::AGENCY_PICKUP)));
    REQUIRE(f.app.registerPackage(paquete("ND-2", PaymentType::PREPAID,
                                          DeliveryMethod::HOME_DELIVERY)));

    CHECK(f.app.updatePackageStatus("ND-1", PackageStatus::RETURNED));

    REQUIRE(f.app.updatePackageStatus("ND-2", PackageStatus::OUT_FOR_DELIVERY));
    CHECK(f.app.updatePackageStatus("ND-2", PackageStatus::RETURNED));
    CHECK(f.app.trackPackage("ND-2")->status == PackageStatus::RETURNED);

    // Un devuelto no vuelve a moverse.
    CHECK_FALSE(f.app.updatePackageStatus("ND-2", PackageStatus::IN_TRANSIT));
}

TEST_CASE("5. cancelPackage: solo desde REGISTERED (S3.4)")
{
    Fixture f;

    Package registrado = paquete("ND-1", PaymentType::PREPAID, DeliveryMethod::AGENCY_PICKUP);
    registrado.status = PackageStatus::REGISTERED;
    REQUIRE(f.app.registerPackage(registrado));
    CHECK(f.app.cancelPackage("ND-1"));
    CHECK(f.app.trackPackage("ND-1")->status == PackageStatus::RETURNED);

    // En transito ya no se cancela. El paquete se registra en REGISTERED: la
    // fabrica de pruebas lo crea en AT_LOCAL_AGENCY.
    Package enTransito = paquete("ND-2", PaymentType::PREPAID, DeliveryMethod::AGENCY_PICKUP);
    enTransito.status = PackageStatus::REGISTERED;
    REQUIRE(f.app.registerPackage(enTransito));
    REQUIRE(f.app.updatePackageStatus("ND-2", PackageStatus::IN_TRANSIT));
    CHECK_FALSE(f.app.cancelPackage("ND-2"));

    CHECK_FALSE(f.app.cancelPackage("ND-404"));
}

} // TEST_SUITE
