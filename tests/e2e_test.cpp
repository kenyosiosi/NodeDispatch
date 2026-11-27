// -----------------------------------------------------------------------------
// NodeDispatch - Prueba end-to-end del MVP (S3.9)
//
// Recorre el flujo completo por la fachada, sin tocar modulos internos:
// registrar -> rastrear -> pagar -> despachar -> entregar.
//
// Dos casos: prepago completo y Cobro Destino bloqueado hasta pagar. El segundo
// falla si alguien rompe la regla COD en cualquier capa (fachada, engine o cola).
// -----------------------------------------------------------------------------

#include <string>

#include <doctest/doctest.h>

#include "nodedispatch/node_dispatch.hpp"

using namespace nodedispatch;

namespace {

Client cliente(const std::string& id, const std::string& nombre)
{
    Client client;
    client.nationalId = id;
    client.firstName = nombre;
    client.lastName = "Perez";
    client.phone = "+58-412-0000000";
    client.address.state = "Distrito Capital";
    client.address.city = "Caracas";
    client.address.exactAddress = "Calle 1";
    client.address.postalCode = "1010";
    return client;
}

Agency agencia(const std::string& id, const std::string& nombre)
{
    Agency agency;
    agency.id = id;
    agency.name = nombre;
    agency.location.state = "Distrito Capital";
    agency.location.city = "Caracas";
    agency.location.exactAddress = "Av. 1";
    agency.location.postalCode = "1010";
    return agency;
}

Package paquete(const std::string& code, PaymentType tipo, DeliveryMethod metodo,
                ServiceLevel nivel, std::int64_t createdAt)
{
    Package package;
    package.trackingCode = code;
    package.senderId = "V-1";
    package.receiverId = "V-2";
    package.targetAgencyId = "AG-CCS-CH";
    package.weightKg = 2.5;
    package.baseCents = 1500;
    package.deliverySurchargeCents = 300;
    package.paymentType = tipo;
    package.deliveryMethod = metodo;
    package.serviceLevel = nivel;
    package.status = PackageStatus::REGISTERED;
    package.createdAt = createdAt;
    return package;
}

// Escenario comun: una raiz, una agencia hija y dos clientes.
struct Escenario {
    NodeDispatch app;
    Escenario()
    {
        app.addAgency(agencia("AG-CCS", "Caracas"), "");
        app.addAgency(agencia("AG-CCS-CH", "Chacao"), "AG-CCS");
        app.registerClient(cliente("V-1", "Ana"));
        app.registerClient(cliente("V-2", "Luis"));
    }
};

} // namespace

TEST_SUITE("dominio/e2e") {

TEST_CASE("flujo completo de un prepago: registrar, rastrear, despachar y entregar (S3.9)")
{
    Escenario e;

    // 1. Registrar.
    REQUIRE(e.app.registerPackage(paquete("ND-000001", PaymentType::PREPAID,
                                          DeliveryMethod::HOME_DELIVERY,
                                          ServiceLevel::EXPRESS_24H, 1795000000)));

    // 2. Rastrear: nace en REGISTERED y ya pagado.
    const auto recien = e.app.trackPackage("ND-000001");
    REQUIRE(recien.has_value());
    CHECK(recien->status == PackageStatus::REGISTERED);
    CHECK(recien->paymentStatus == PaymentStatus::PAID);

    // 3. Viaje a la agencia destino.
    REQUIRE(e.app.updatePackageStatus("ND-000001", PackageStatus::IN_TRANSIT));
    REQUIRE(e.app.updatePackageStatus("ND-000001", PackageStatus::AT_LOCAL_AGENCY));

    // 4. Despachar: a domicilio, sale a reparto.
    const auto despachado = e.app.dispatchNextPackage("AG-CCS-CH");
    REQUIRE(despachado.has_value());
    CHECK(despachado->trackingCode == "ND-000001");
    CHECK(despachado->status == PackageStatus::OUT_FOR_DELIVERY);

    // 5. Entregar.
    REQUIRE(e.app.updatePackageStatus("ND-000001", PackageStatus::DELIVERED));
    CHECK(e.app.trackPackage("ND-000001")->status == PackageStatus::DELIVERED);

    // El ingreso cuenta base + recargo de ultima milla.
    CHECK(e.app.totalRevenueCents() == 1800);

    // Y la agencia queda vacia: el paquete ya salio.
    CHECK(e.app.packagesAtAgency("AG-CCS-CH").is_empty());
}

TEST_CASE("Cobro Destino queda bloqueado hasta pagar (S3.9)")
{
    Escenario e;

    REQUIRE(e.app.registerPackage(paquete("ND-000002", PaymentType::CASH_ON_DELIVERY,
                                          DeliveryMethod::AGENCY_PICKUP,
                                          ServiceLevel::EXPRESS_24H, 1795000001)));

    // Nace pendiente de cobro, pese a ser el mas urgente de la cola.
    CHECK(e.app.trackPackage("ND-000002")->paymentStatus == PaymentStatus::PENDING);

    REQUIRE(e.app.updatePackageStatus("ND-000002", PackageStatus::IN_TRANSIT));
    REQUIRE(e.app.updatePackageStatus("ND-000002", PackageStatus::AT_LOCAL_AGENCY));

    // Sin pagar no se despacha: la regla muerde aqui.
    CHECK_FALSE(e.app.dispatchNextPackage("AG-CCS-CH").has_value());
    CHECK(e.app.trackPackage("ND-000002")->status == PackageStatus::AT_LOCAL_AGENCY);

    // Tampoco se puede forzar la entrega por la puerta de atras.
    CHECK_FALSE(e.app.updatePackageStatus("ND-000002", PackageStatus::DELIVERED));

    // Se cobra.
    REQUIRE(e.app.updatePaymentStatus("ND-000002", PaymentStatus::PAID));

    // Ahora si sale, y al ser retiro en agencia se entrega directamente.
    const auto despachado = e.app.dispatchNextPackage("AG-CCS-CH");
    REQUIRE(despachado.has_value());
    CHECK(despachado->trackingCode == "ND-000002");
    CHECK(despachado->status == PackageStatus::DELIVERED);

    // Solo despues de pagar aparece en los ingresos.
    CHECK(e.app.totalRevenueCents() == 1500);
    CHECK(e.app.packagesByStatus(PackageStatus::DELIVERED).size() == 1);
}

TEST_CASE("el despacho respeta el orden de servicio en un escenario real (S3.9)")
{
    Escenario e;

    // Tres paquetes en la agencia, con niveles distintos y llegada distinta.
    const Package express = paquete("ND-E", PaymentType::PREPAID,
                                    DeliveryMethod::AGENCY_PICKUP,
                                    ServiceLevel::EXPRESS_24H, 1795000100);
    const Package standard = paquete("ND-S", PaymentType::PREPAID,
                                     DeliveryMethod::AGENCY_PICKUP,
                                     ServiceLevel::STANDARD, 1795000000);
    const Package priority = paquete("ND-P", PaymentType::PREPAID,
                                     DeliveryMethod::AGENCY_PICKUP,
                                     ServiceLevel::PRIORITY, 1795000050);

    for (const Package& p : {standard, express, priority})
    {
        REQUIRE(e.app.registerPackage(p));
        REQUIRE(e.app.updatePackageStatus(p.trackingCode, PackageStatus::IN_TRANSIT));
        REQUIRE(e.app.updatePackageStatus(p.trackingCode, PackageStatus::AT_LOCAL_AGENCY));
    }

    CHECK(e.app.dispatchNextPackage("AG-CCS-CH")->trackingCode == "ND-E");
    CHECK(e.app.dispatchNextPackage("AG-CCS-CH")->trackingCode == "ND-P");
    CHECK(e.app.dispatchNextPackage("AG-CCS-CH")->trackingCode == "ND-S");
    CHECK_FALSE(e.app.dispatchNextPackage("AG-CCS-CH").has_value());

    // Los tres quedaron entregados (retiro) y sumaron ingresos.
    CHECK(e.app.packagesByStatus(PackageStatus::DELIVERED).size() == 3);
    CHECK(e.app.totalRevenueCents() == 4500);
}

} // TEST_SUITE
