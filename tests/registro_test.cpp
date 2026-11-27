// -----------------------------------------------------------------------------
// NodeDispatch - registerPackage y doble indexacion (S3.1)
//
// Seis rechazos y dos normalizaciones, mas la comprobacion de que un paquete
// que nace en la agencia queda en el inventario ademas de en el engine.
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

Package paquete(const std::string& code)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = "AG-CCS";
    package.weightKg = 1.0;
    package.baseCents = 1500;
    package.deliverySurchargeCents = 300;
    package.paymentType = PaymentType::PREPAID;
    package.deliveryMethod = DeliveryMethod::AGENCY_PICKUP;
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
        app.addAgency(agencia("AG-VAL"), "AG-CCS");
        app.registerClient(cliente("V-1"));
        app.registerClient(cliente("V-2"));
    }
};

} // namespace

TEST_SUITE("dominio/registro") {

TEST_CASE("rechazo 1: codigo duplicado (S3.1)")
{
    Fixture f;
    CHECK(f.app.registerPackage(paquete("ND-1")));
    CHECK_FALSE(f.app.registerPackage(paquete("ND-1")));
}

TEST_CASE("rechazo 2: peso no positivo (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.weightKg = 0.0;
    CHECK_FALSE(f.app.registerPackage(p));
}

TEST_CASE("rechazo 3: importe negativo (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.baseCents = -1;
    CHECK_FALSE(f.app.registerPackage(p));
}

TEST_CASE("rechazo 4: agencia destino inexistente (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.targetAgencyId = "AG-404";
    CHECK_FALSE(f.app.registerPackage(p));
}

TEST_CASE("rechazo 5: remitente no registrado (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.senderId = "V-404";
    CHECK_FALSE(f.app.registerPackage(p));
}

TEST_CASE("rechazo 6: destinatario no registrado (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.receiverId = "V-404";
    CHECK_FALSE(f.app.registerPackage(p));
}

TEST_CASE("normalizacion 1: PREPAID nace PAID (S3.1)")
{
    Fixture f;
    CHECK(f.app.registerPackage(paquete("ND-1")));
    CHECK(f.app.trackPackage("ND-1")->paymentStatus == PaymentStatus::PAID);
}

TEST_CASE("normalizacion 2: CASH_ON_DELIVERY nace PENDING (S3.1)")
{
    Fixture f;
    Package p = paquete("ND-1");
    p.paymentType = PaymentType::CASH_ON_DELIVERY;
    p.paymentStatus = PaymentStatus::PAID; // se sobrescribe con la regla
    CHECK(f.app.registerPackage(p));
    CHECK(f.app.trackPackage("ND-1")->paymentStatus == PaymentStatus::PENDING);
}

TEST_CASE("doble indexacion: el paquete nace en el engine y en el inventario (S3.1)")
{
    Fixture f;

    Package p = paquete("ND-1");
    p.status = PackageStatus::AT_LOCAL_AGENCY;
    REQUIRE(f.app.registerPackage(p));

    // En el engine.
    CHECK(f.app.trackPackage("ND-1").has_value());
    // Y en el inventario de su agencia, que es stock presente.
    const auto enAgencia = f.app.packagesAtAgency("AG-CCS");
    REQUIRE(enAgencia.size() == 1);
    CHECK(enAgencia[0].trackingCode == "ND-1");

    // En cambio, un paquete que nace REGISTERED no esta en el inventario.
    REQUIRE(f.app.registerPackage(paquete("ND-2")));
    CHECK(f.app.packagesAtAgency("AG-CCS").size() == 1);
}

} // TEST_SUITE
