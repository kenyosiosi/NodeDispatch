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

#include <string>
#include <string_view>
#include <type_traits>

#include <doctest/doctest.h>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/client.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

using namespace nodedispatch;

namespace {

// --- SDD 4.1 y OperationResult: los enum class de types.hpp ----------------
// Los seis enums del SDD 4.1 mas OperationResult, que agrega la fachada.
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
static_assert(isScopedEnum<OperationResult>(), "OperationResult debe ser enum class");

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

// --- toMessage es constexpr -----------------------------------------------
// Permite usarlo en static_assert y en contextos de compilacion.
static_assert(std::string_view(toMessage(OperationResult::OK)) == "Operacion completada",
              "toMessage debe ser constexpr y cubrir OK");

// --- SDD 4.2 y 4.3: tipo de cada campo --------------------------------------
// El texto va en std::string (los char[N] rompen HashMap: comparan direcciones)
// y el dinero en centavos enteros. Si alguien cambia un campo por `double` o
// por `char[]`, esto no compila.
static_assert(std::is_same<decltype(Address::state), std::string>::value,
              "Address::state debe ser std::string (SDD 4.3)");
static_assert(std::is_same<decltype(Address::city), std::string>::value,
              "Address::city debe ser std::string (SDD 4.3)");
static_assert(std::is_same<decltype(Address::exactAddress), std::string>::value,
              "Address::exactAddress debe ser std::string (SDD 4.3)");
static_assert(std::is_same<decltype(Address::postalCode), std::string>::value,
              "Address::postalCode debe ser std::string (SDD 4.3)");

static_assert(std::is_same<decltype(Agency::id), std::string>::value,
              "Agency::id debe ser std::string (SDD 4.2)");
static_assert(std::is_same<decltype(Agency::parentId), std::string>::value,
              "Agency::parentId debe ser std::string (SDD 4.2)");
static_assert(std::is_same<decltype(Agency::location), Address>::value,
              "Agency::location debe ser Address (SDD 4.2)");

static_assert(std::is_same<decltype(Client::nationalId), std::string>::value,
              "Client::nationalId debe ser std::string (SDD 4.2)");

static_assert(std::is_same<decltype(Package::baseCents), std::int64_t>::value,
              "Package::baseCents debe ser int64_t en centavos (SDD 4.3)");
static_assert(std::is_same<decltype(Package::deliverySurchargeCents), std::int64_t>::value,
              "Package::deliverySurchargeCents debe ser int64_t en centavos (SDD 4.3)");
static_assert(std::is_same<decltype(Package::createdAt), std::int64_t>::value,
              "Package::createdAt debe ser int64_t (SDD 4.2)");
static_assert(std::is_same<decltype(Package::updatedAt), std::int64_t>::value,
              "Package::updatedAt debe ser int64_t (SDD 4.2)");
static_assert(std::is_same<decltype(Package::trackingCode), std::string>::value,
              "Package::trackingCode debe ser std::string (SDD 4.2)");

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

    // Estado neutro previo al registro: un Package{} por defecto todavia no es
    // un paquete valido. registerPackage aplica la regla del SDD 5.2.
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

TEST_CASE("toMessage cubre los cinco valores de OperationResult (S1.5)")
{
    // Los mensajes son parte del contrato con el usuario: se fijan aqui para
    // que un cambio de texto no pase desapercibido.
    CHECK(std::string(toMessage(OperationResult::OK)) == "Operacion completada");
    CHECK(std::string(toMessage(OperationResult::NOT_FOUND)) == "No encontrado");
    CHECK(std::string(toMessage(OperationResult::DUPLICATE)) == "Ya existe");
    CHECK(std::string(toMessage(OperationResult::INVALID_STATE)) == "Estado invalido");
    CHECK(std::string(toMessage(OperationResult::INVALID_INPUT)) == "Entrada invalida");

    // Dos codigos distintos no pueden mostrar el mismo texto: el usuario no
    // sabria que ocurrio.
    const OperationResult codes[] = {OperationResult::OK, OperationResult::NOT_FOUND,
                                     OperationResult::DUPLICATE, OperationResult::INVALID_STATE,
                                     OperationResult::INVALID_INPUT};

    for (const OperationResult a : codes)
    {
        for (const OperationResult b : codes)
        {
            if (a == b)
                continue;
            CHECK(std::string(toMessage(a)) != std::string(toMessage(b)));
        }
    }
}

TEST_CASE("los structs del dominio mantienen el tamano documentado (S1.4)")
{
    // Cifras de referencia medidas en x86-64 con GCC 13.3 (std::string = 32 B):
    // Address 128, Client 256, Agency 224, Package 192. El SDD 4.3 las recoge.
    // Aqui se expresan en funcion de sizeof(std::string) para que valgan en
    // cualquier ABI y sigan detectando un campo de mas o de menos.
    MESSAGE("sizeof(Address)=" << sizeof(Address) << " Client=" << sizeof(Client)
            << " Agency=" << sizeof(Agency) << " Package=" << sizeof(Package));

    CHECK(sizeof(Address) == 4 * sizeof(std::string));
    CHECK(sizeof(Client) == 8 * sizeof(std::string));
    CHECK(sizeof(Agency) == 7 * sizeof(std::string));
    CHECK(sizeof(Package) >= 4 * sizeof(std::string) + 2 * sizeof(std::int64_t));

    // El peso es la unica magnitud en punto flotante del dominio (no es dinero).
    static_assert(std::is_same<decltype(Package::weightKg), double>::value,
                  "Package::weightKg debe ser double (SDD 4.2)");
}

} // TEST_SUITE
