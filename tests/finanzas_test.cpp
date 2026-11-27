// -----------------------------------------------------------------------------
// NodeDispatch - Calculo financiero y regla COD (S3.6)
//
// El importe total se calcula en centavos enteros (SDD 5.2) y el Cobro Destino
// no se entrega sin pagar. Se prueba a traves de la fachada para cubrir la
// orquestacion completa, no solo la formula.
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

Package paquete(const std::string& code, DeliveryMethod method, PaymentType type)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = "AG-CCS";
    package.weightKg = 1.0;
    package.baseCents = 1500;
    package.deliverySurchargeCents = 300;
    package.paymentType = type;
    package.deliveryMethod = method;
    package.serviceLevel = ServiceLevel::STANDARD;
    package.status = PackageStatus::REGISTERED;
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

TEST_SUITE("dominio/finanzas") {

TEST_CASE("el retiro en agencia no cobra recargo (S3.6)")
{
    Package p = paquete("ND-1", DeliveryMethod::AGENCY_PICKUP, PaymentType::PREPAID);
    CHECK(totalCents(p) == 1500);
}

TEST_CASE("la entrega a domicilio suma el recargo (S3.6)")
{
    Package p = paquete("ND-1", DeliveryMethod::HOME_DELIVERY, PaymentType::PREPAID);
    CHECK(totalCents(p) == 1800);

    // El recargo es del paquete, no de la formula: cambiarlo cambia el total.
    p.deliverySurchargeCents = 0;
    CHECK(totalCents(p) == 1500);
}

TEST_CASE("Cobro Destino impago no se entrega (S3.6)")
{
    Fixture f;

    REQUIRE(f.app.registerPackage(paquete("ND-COD", DeliveryMethod::AGENCY_PICKUP,
                                          PaymentType::CASH_ON_DELIVERY)));
    REQUIRE(f.app.updatePackageStatus("ND-COD", PackageStatus::IN_TRANSIT));
    REQUIRE(f.app.updatePackageStatus("ND-COD", PackageStatus::AT_LOCAL_AGENCY));

    CHECK_FALSE(f.app.updatePackageStatus("ND-COD", PackageStatus::DELIVERED));
    CHECK(f.app.trackPackage("ND-COD")->status == PackageStatus::AT_LOCAL_AGENCY);

    // Pagado, la misma transicion pasa.
    REQUIRE(f.app.updatePaymentStatus("ND-COD", PaymentStatus::PAID));
    CHECK(f.app.updatePackageStatus("ND-COD", PackageStatus::DELIVERED));
}

TEST_CASE("un pago fallido deja el paquete elegible para devolucion (S3.6)")
{
    Fixture f;

    REQUIRE(f.app.registerPackage(paquete("ND-COD", DeliveryMethod::AGENCY_PICKUP,
                                          PaymentType::CASH_ON_DELIVERY)));
    REQUIRE(f.app.updatePackageStatus("ND-COD", PackageStatus::IN_TRANSIT));
    REQUIRE(f.app.updatePackageStatus("ND-COD", PackageStatus::AT_LOCAL_AGENCY));
    REQUIRE(f.app.updatePaymentStatus("ND-COD", PaymentStatus::FAILED));

    CHECK(f.app.updatePackageStatus("ND-COD", PackageStatus::RETURNED));
    CHECK(f.app.trackPackage("ND-COD")->status == PackageStatus::RETURNED);

    // Y el ingreso no lo cuenta: nunca se cobro.
    CHECK(f.app.totalRevenueCents() == 0);
}

} // TEST_SUITE
