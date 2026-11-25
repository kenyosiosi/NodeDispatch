// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas de la fachada (N3)
//
// Cubre la orquestacion: validaciones de registerPackage, normalizacion del
// pago, seguimiento del inventario y la cola, despacho y reportes.
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/node_dispatch.hpp"

using namespace nodedispatch;

namespace {

Client makeClient(const std::string& id, const std::string& name = "Ana")
{
    Client client;
    client.nationalId = id;
    client.firstName = name;
    client.lastName = "Perez";
    client.phone = "+58-412-0000000";
    client.address.state = "Distrito Capital";
    client.address.city = "Caracas";
    client.address.exactAddress = "Calle 1";
    client.address.postalCode = "1010";
    return client;
}

Agency makeAgency(const std::string& id, const std::string& name = "Agencia")
{
    Agency agency;
    agency.id = id;
    agency.name = name;
    agency.location.state = "Distrito Capital";
    agency.location.city = "Caracas";
    agency.location.exactAddress = "Av. 1";
    agency.location.postalCode = "1010";
    return agency;
}

Package makePackage(const std::string& code, const std::string& agency = "AG-CCS",
                    PaymentType type = PaymentType::PREPAID,
                    DeliveryMethod method = DeliveryMethod::AGENCY_PICKUP,
                    PackageStatus status = PackageStatus::REGISTERED)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = agency;
    package.weightKg = 2.0;
    package.baseCents = 1500;
    package.deliverySurchargeCents = 300;
    package.paymentType = type;
    package.deliveryMethod = method;
    package.serviceLevel = ServiceLevel::STANDARD;
    package.status = status;
    package.createdAt = 1795000000;
    package.updatedAt = 1795000000;
    return package;
}

// Fachada con una agencia raiz y dos clientes ya registrados.
struct Fixture {
    NodeDispatch app;

    Fixture()
    {
        app.addAgency(makeAgency("AG-CCS", "Caracas"), "");
        app.addAgency(makeAgency("AG-CCS-CH", "Chacao"), "AG-CCS");
        app.registerClient(makeClient("V-1", "Ana"));
        app.registerClient(makeClient("V-2", "Luis"));
    }
};

} // namespace

TEST_SUITE("dominio/fachada") {

TEST_CASE("la fachada administra la jerarquia de agencias (N3)")
{
    Fixture f;

    CHECK(f.app.findAgency("AG-CCS").has_value());
    CHECK_FALSE(f.app.findAgency("AG-404").has_value());

    // Raiz repetida y padre inexistente: rechazados por la jerarquia.
    CHECK_FALSE(f.app.addAgency(makeAgency("AG-CCS"), ""));
    CHECK_FALSE(f.app.addAgency(makeAgency("AG-X"), "AG-404"));
    CHECK_FALSE(f.app.addAgency(makeAgency(""), ""));

    const auto children = f.app.listDescendants("AG-CCS");
    REQUIRE(children.size() == 1);
    CHECK(children[0].id == "AG-CCS-CH");
}

TEST_CASE("registerPackage valida el paquete antes de aceptarlo (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-1")));

    // Codigo duplicado.
    CHECK_FALSE(f.app.registerPackage(makePackage("ND-1")));

    // Peso no positivo.
    Package sinPeso = makePackage("ND-2");
    sinPeso.weightKg = 0.0;
    CHECK_FALSE(f.app.registerPackage(sinPeso));

    // Importe negativo.
    Package negativo = makePackage("ND-3");
    negativo.baseCents = -1;
    CHECK_FALSE(f.app.registerPackage(negativo));

    // Agencia inexistente.
    CHECK_FALSE(f.app.registerPackage(makePackage("ND-4", "AG-404")));

    // Cliente no registrado.
    Package extrano = makePackage("ND-5");
    extrano.senderId = "V-404";
    CHECK_FALSE(f.app.registerPackage(extrano));

    // Un paquete rechazado no queda en el sistema.
    CHECK_FALSE(f.app.trackPackage("ND-3").has_value());
}

TEST_CASE("registerPackage normaliza el pago segun la tabla 5.2 (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-PRE", "AG-CCS", PaymentType::PREPAID)));
    CHECK(f.app.trackPackage("ND-PRE")->paymentStatus == PaymentStatus::PAID);

    CHECK(f.app.registerPackage(makePackage("ND-COD", "AG-CCS", PaymentType::CASH_ON_DELIVERY)));
    CHECK(f.app.trackPackage("ND-COD")->paymentStatus == PaymentStatus::PENDING);
}

TEST_CASE("el paquete generado sin codigo obtiene uno del generador (N3)")
{
    Fixture f;

    Package sinCodigo = makePackage("");
    CHECK(f.app.registerPackage(sinCodigo));

    const auto vacios = f.app.packagesByStatus(PackageStatus::REGISTERED);
    REQUIRE(vacios.size() == 1);
    CHECK(vacios[0].trackingCode.rfind("ND-", 0) == 0);
    CHECK(vacios[0].trackingCode.size() == 18);
}

TEST_CASE("despachar entrega el retiro en agencia y lo saca del inventario (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-1", "AG-CCS", PaymentType::PREPAID,
                                            DeliveryMethod::AGENCY_PICKUP,
                                            PackageStatus::AT_LOCAL_AGENCY)));

    CHECK(f.app.packagesAtAgency("AG-CCS").size() == 1);

    const auto dispatched = f.app.dispatchNextPackage("AG-CCS");
    REQUIRE(dispatched.has_value());
    CHECK(dispatched->trackingCode == "ND-1");
    CHECK(dispatched->status == PackageStatus::DELIVERED);

    // Ya no esta en el inventario ni se puede volver a despachar.
    CHECK(f.app.packagesAtAgency("AG-CCS").size() == 0);
    CHECK_FALSE(f.app.dispatchNextPackage("AG-CCS").has_value());
}

TEST_CASE("el despacho a domicilio pasa a reparto (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-1", "AG-CCS", PaymentType::PREPAID,
                                            DeliveryMethod::HOME_DELIVERY,
                                            PackageStatus::AT_LOCAL_AGENCY)));

    const auto dispatched = f.app.dispatchNextPackage("AG-CCS");
    REQUIRE(dispatched.has_value());
    CHECK(dispatched->status == PackageStatus::OUT_FOR_DELIVERY);
}

TEST_CASE("Cobro Destino sin pagar no se despacha y no bloquea a los demas (N3)")
{
    Fixture f;

    // El COD es mas urgente, pero no puede salir: no debe bloquear al prepago.
    Package cod = makePackage("ND-COD", "AG-CCS", PaymentType::CASH_ON_DELIVERY,
                              DeliveryMethod::AGENCY_PICKUP,
                              PackageStatus::AT_LOCAL_AGENCY);
    cod.serviceLevel = ServiceLevel::EXPRESS_24H;
    cod.createdAt = 1795000000;
    REQUIRE(f.app.registerPackage(cod));

    Package prepago = makePackage("ND-PRE", "AG-CCS", PaymentType::PREPAID,
                                  DeliveryMethod::AGENCY_PICKUP,
                                  PackageStatus::AT_LOCAL_AGENCY);
    prepago.createdAt = 1795000001;
    REQUIRE(f.app.registerPackage(prepago));

    const auto first = f.app.dispatchNextPackage("AG-CCS");
    REQUIRE(first.has_value());
    CHECK(first->trackingCode == "ND-PRE"); // el COD se aparta y vuelve a la cola

    // Se cobra: ahora si sale.
    REQUIRE(f.app.updatePaymentStatus("ND-COD", PaymentStatus::PAID));
    const auto second = f.app.dispatchNextPackage("AG-CCS");
    REQUIRE(second.has_value());
    CHECK(second->trackingCode == "ND-COD");
}

TEST_CASE("cancelar solo es posible antes del envio (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-1")));
    CHECK(f.app.cancelPackage("ND-1"));
    CHECK(f.app.trackPackage("ND-1")->status == PackageStatus::RETURNED);

    // Un paquete ya devuelto no se cancela dos veces.
    CHECK_FALSE(f.app.cancelPackage("ND-1"));

    // Y uno en transito tampoco.
    CHECK(f.app.registerPackage(makePackage("ND-2")));
    REQUIRE(f.app.updatePackageStatus("ND-2", PackageStatus::IN_TRANSIT));
    CHECK_FALSE(f.app.cancelPackage("ND-2"));
}

TEST_CASE("reportes: por estado, por agencia y ingresos (N3)")
{
    Fixture f;

    CHECK(f.app.registerPackage(makePackage("ND-1", "AG-CCS", PaymentType::PREPAID,
                                            DeliveryMethod::AGENCY_PICKUP)));
    CHECK(f.app.registerPackage(makePackage("ND-2", "AG-CCS", PaymentType::CASH_ON_DELIVERY,
                                            DeliveryMethod::AGENCY_PICKUP)));

    CHECK(f.app.packagesByStatus(PackageStatus::REGISTERED).size() == 2);

    // Solo el prepago esta pagado: 1500 centavos (sin recargo, retiro).
    CHECK(f.app.totalRevenueCents() == 1500);

    CHECK(f.app.updatePaymentStatus("ND-2", PaymentStatus::PAID));
    CHECK(f.app.totalRevenueCents() == 3000);

    // El domicilio suma el recargo.
    CHECK(f.app.registerPackage(makePackage("ND-3", "AG-CCS", PaymentType::PREPAID,
                                            DeliveryMethod::HOME_DELIVERY)));
    CHECK(f.app.totalRevenueCents() == 3000 + 1500 + 300);
}

} // TEST_SUITE
