// -----------------------------------------------------------------------------
// NodeDispatch - S1.3 Invariantes de los tipos del dominio
//
// Fija por contrato lo que el SDD 4.1 define como enumeraciones del dominio y
// los valores por defecto de la implementacion (S1.0) para Package, Agency y
// Client. El SDD 4.2 solo declara los structs, no sus valores por defecto.
//
// Los enums se comprueban con static_assert: cualquier cambio de un valor
// rompe la compilacion. Los valores por defecto se comprueban en tiempo de
// ejecucion porque Package, Agency y Client contienen std::string, que no es
// constexpr en C++17 y por lo tanto no puede aparecer en un static_assert.
// -----------------------------------------------------------------------------

#include <type_traits>

#include <doctest/doctest.h>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/client.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

using namespace nodedispatch;

namespace {

// --- SDD 4.1: los seis enums del dominio son enum class ----------------------
// Un enum class no se convierte implicitamente a entero; uno sin `class` si.
template <typename T>
constexpr bool isScopedEnum()
{
    return std::is_enum<T>::value && !std::is_convertible<T, int>::value;
}

static_assert(isScopedEnum<PaymentType>(), "PaymentType debe ser enum class");
static_assert(isScopedEnum<PaymentStatus>(), "PaymentStatus debe ser enum class");
static_assert(isScopedEnum<DeliveryMethod>(), "DeliveryMethod debe ser enum class");
static_assert(isScopedEnum<ServiceLevel>(), "ServiceLevel debe ser enum class");
static_assert(isScopedEnum<PackageStatus>(), "PackageStatus debe ser enum class");
static_assert(isScopedEnum<ReturnReason>(), "ReturnReason debe ser enum class");

// --- SDD 4.1 y 5.3: ServiceLevel vale 1, 2, 3 en ese orden -------------------
// DispatchCmp compara los niveles por su valor entero, asi que el orden
// numerico es parte del contrato de prioridad de despacho.
static_assert(static_cast<int>(ServiceLevel::EXPRESS_24H) == 1,
              "EXPRESS_24H debe valer 1 (SDD 4.1)");
static_assert(static_cast<int>(ServiceLevel::PRIORITY) == 2,
              "PRIORITY debe valer 2 (SDD 4.1)");
static_assert(static_cast<int>(ServiceLevel::STANDARD) == 3,
              "STANDARD debe valer 3 (SDD 4.1)");

static_assert(static_cast<int>(ServiceLevel::EXPRESS_24H) <
                  static_cast<int>(ServiceLevel::PRIORITY),
              "EXPRESS_24H debe ser mas urgente que PRIORITY (SDD 5.3)");
static_assert(static_cast<int>(ServiceLevel::PRIORITY) <
                  static_cast<int>(ServiceLevel::STANDARD),
              "PRIORITY debe ser mas urgente que STANDARD (SDD 5.3)");

// --- Los tipos de valor son construibles por defecto ------------------------
static_assert(std::is_default_constructible<Package>::value,
              "Package debe poder construirse por defecto");
static_assert(std::is_default_constructible<Agency>::value,
              "Agency debe poder construirse por defecto");
static_assert(std::is_default_constructible<Client>::value,
              "Client debe poder construirse por defecto");

} // namespace

TEST_SUITE("dominio/types") {

TEST_CASE("Package nace con los valores por defecto de la implementacion (S1.0)")
{
    const Package pkg;

    CHECK(pkg.trackingCode.empty());
    CHECK(pkg.senderId.empty());
    CHECK(pkg.receiverId.empty());
    CHECK(pkg.targetAgencyId.empty());

    CHECK(pkg.weightKg == doctest::Approx(0.0));
    CHECK(pkg.baseCents == 0);
    CHECK(pkg.deliverySurchargeCents == 0);

    // SDD 5.2: el prepago es el modo por defecto y el pago arranca pendiente.
    CHECK(pkg.paymentType == PaymentType::PREPAID);
    CHECK(pkg.paymentStatus == PaymentStatus::PENDING);

    CHECK(pkg.deliveryMethod == DeliveryMethod::AGENCY_PICKUP);
    CHECK(pkg.serviceLevel == ServiceLevel::STANDARD);
    CHECK(pkg.status == PackageStatus::REGISTERED);
    CHECK(pkg.returnReason == ReturnReason::NONE);

    CHECK(pkg.createdAt == 0);
    CHECK(pkg.updatedAt == 0);
}

TEST_CASE("Agency nace vacia y sin padre")
{
    const Agency agency;

    CHECK(agency.id.empty());
    CHECK(agency.name.empty());
    CHECK(agency.parentId.empty()); // SDD 4.2: vacio para la raiz
    CHECK(agency.location.state.empty());
    CHECK(agency.location.city.empty());
    CHECK(agency.location.exactAddress.empty());
    CHECK(agency.location.postalCode.empty());
}

TEST_CASE("Client nace vacio")
{
    const Client client;

    CHECK(client.nationalId.empty());
    CHECK(client.firstName.empty());
    CHECK(client.lastName.empty());
    CHECK(client.phone.empty());
    CHECK(client.address.state.empty());
    CHECK(client.address.city.empty());
    CHECK(client.address.exactAddress.empty());
    CHECK(client.address.postalCode.empty());
}

} // TEST_SUITE
