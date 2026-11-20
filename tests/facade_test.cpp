// -----------------------------------------------------------------------------
// NodeDispatch - S1.6 Contrato publico de la fachada
//
// El SDD 7 define 15 metodos y este archivo los fija por contrato. La prueba es
// casi toda en tiempo de compilacion: si alguien cambia un tipo de retorno, un
// `const` o el orden de los parametros, la compilacion falla aqui y no tres
// modulos mas adelante.
//
// No se invoca ningun metodo: en S1.6 la fachada es solo contrato y sus
// definiciones llegan con los modulos (N3). Instanciar la clase comprueba que
// sigue siendo construible por defecto.
// -----------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include <doctest/doctest.h>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/client.hpp"
#include "nodedispatch/node_dispatch.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

using namespace nodedispatch;

namespace {

// --- Jerarquia geografica ---------------------------------------------------
using AddAgency = bool (NodeDispatch::*)(const Agency&, const std::string&);
static_assert(std::is_same<decltype(&NodeDispatch::addAgency), AddAgency>::value,
              "addAgency debe devolver bool y recibir la agencia y el padre");

using FindAgency = std::optional<Agency> (NodeDispatch::*)(const std::string&) const;
static_assert(std::is_same<decltype(&NodeDispatch::findAgency), FindAgency>::value,
              "findAgency debe devolver una copia opcional y ser const");

using ListDescendants =
    Designar::DynArray<Agency> (NodeDispatch::*)(const std::string&) const;
static_assert(std::is_same<decltype(&NodeDispatch::listDescendants), ListDescendants>::value,
              "listDescendants debe devolver copias, no punteros internos");

// --- Clientes ---------------------------------------------------------------
using RegisterClient = bool (NodeDispatch::*)(const Client&);
static_assert(std::is_same<decltype(&NodeDispatch::registerClient), RegisterClient>::value,
              "registerClient debe devolver bool");

// --- Paquetes ---------------------------------------------------------------
using RegisterPackage = bool (NodeDispatch::*)(const Package&);
static_assert(std::is_same<decltype(&NodeDispatch::registerPackage), RegisterPackage>::value,
              "registerPackage debe devolver bool");

using TrackPackage = std::optional<Package> (NodeDispatch::*)(const std::string&) const;
static_assert(std::is_same<decltype(&NodeDispatch::trackPackage), TrackPackage>::value,
              "trackPackage debe devolver std::optional, nunca nullptr");

using UpdatePayment = bool (NodeDispatch::*)(const std::string&, PaymentStatus);
static_assert(std::is_same<decltype(&NodeDispatch::updatePaymentStatus), UpdatePayment>::value,
              "updatePaymentStatus debe devolver bool");

using UpdateStatus = bool (NodeDispatch::*)(const std::string&, PackageStatus);
static_assert(std::is_same<decltype(&NodeDispatch::updatePackageStatus), UpdateStatus>::value,
              "updatePackageStatus debe devolver bool");

using CancelPackage = bool (NodeDispatch::*)(const std::string&);
static_assert(std::is_same<decltype(&NodeDispatch::cancelPackage), CancelPackage>::value,
              "cancelPackage debe devolver bool");

// --- Despacho ---------------------------------------------------------------
using DispatchNext = std::optional<Package> (NodeDispatch::*)(const std::string&);
static_assert(std::is_same<decltype(&NodeDispatch::dispatchNextPackage), DispatchNext>::value,
              "dispatchNextPackage debe devolver una copia opcional");

// --- Consultas y reportes ---------------------------------------------------
using PackagesAtAgency = Designar::DynArray<Package> (NodeDispatch::*)(const std::string&) const;
static_assert(std::is_same<decltype(&NodeDispatch::packagesAtAgency), PackagesAtAgency>::value,
              "packagesAtAgency debe devolver copias");

using PackagesByStatus = Designar::DynArray<Package> (NodeDispatch::*)(PackageStatus) const;
static_assert(std::is_same<decltype(&NodeDispatch::packagesByStatus), PackagesByStatus>::value,
              "packagesByStatus debe devolver copias");

using TotalRevenue = std::int64_t (NodeDispatch::*)() const;
static_assert(std::is_same<decltype(&NodeDispatch::totalRevenueCents), TotalRevenue>::value,
              "totalRevenueCents debe devolver centavos en int64_t, nunca double");

// --- Persistencia -----------------------------------------------------------
using SaveFn = bool (NodeDispatch::*)(const std::string&) const;
static_assert(std::is_same<decltype(&NodeDispatch::save), SaveFn>::value,
              "save debe ser const y devolver bool");

using LoadFn = bool (NodeDispatch::*)(const std::string&);
static_assert(std::is_same<decltype(&NodeDispatch::load), LoadFn>::value,
              "load no es const: reconstruye el estado");

// La fachada se construye por defecto; los modulos internos se ensamblan en N3.
static_assert(std::is_default_constructible<NodeDispatch>::value,
              "NodeDispatch debe ser construible por defecto");

} // namespace

TEST_CASE("la fachada NodeDispatch expone el contrato del SDD 7 sin lanzar excepciones")
{
    NodeDispatch facade;
    (void) facade;

    // Un Package por defecto es el estado neutro previo al registro: la fachada
    // no debe aceptarlo como paquete valido (el rechazo se prueba en S3.1).
    const Package neutral;
    CHECK(neutral.trackingCode.empty());
}
