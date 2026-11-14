#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Fachada del nucleo (S1.6)
//
// Contrato publico unico del motor. Los 15 metodos son exactamente los del
// SDD 7 y cubren toda la capa de presentacion del SDD 6.7 (CLI):
//
//   agency add / tree      -> addAgency, findAgency, listDescendants
//   package register       -> registerPackage
//   package track / pay    -> trackPackage, updatePaymentStatus
//   package dispatch       -> dispatchNextPackage, updatePackageStatus
//   report                 -> packagesAtAgency, packagesByStatus,
//                             totalRevenueCents
//   save / load            -> save, load
//
// Fuente unica de verdad de los datos: el TrackingEngine posee el Package.
// InventoryIndex y PriorityDispatchQueue guardan solo codigos de rastreo, de
// modo que un cambio de estado nunca puede quedar aplicado en un sitio y
// desfasado en otro.
//
// Esta cabecera fija solo el CONTRATO: no declara miembros. La composicion de
// los modulos (jerarquia, tracking, inventario, despacho y auditoria) se decide
// en N3, cuando esos tipos existen. Declarar aqui miembros por valor de tipos
// incompletos no compilaria.
//
// Politica de errores (SDD 7): la API publica no lanza excepciones de negocio.
// Las mutaciones devuelven bool; las consultas devuelven std::optional o una
// copia en Designar::DynArray. Nunca se devuelven punteros ni referencias a
// memoria interna: todas las salidas son copias.
// -----------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>

#include <array.hpp>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/client.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class NodeDispatch {
public:
    // --- Jerarquia geografica ----------------------------------------------
    // `parentId` vacio crea la raiz. Rechaza ID vacio, ID duplicado y padre
    // inexistente. La jerarquia es de solo insercion (ver B7).
    bool addAgency(const Agency& agency, const std::string& parentId);

    // Copia de la agencia; nullopt si no existe.
    std::optional<Agency> findAgency(const std::string& id) const;

    // Descendientes en preorden DFS. El nodo consultado no se incluye.
    Designar::DynArray<Agency> listDescendants(const std::string& id) const;

    // --- Clientes -----------------------------------------------------------
    // Rechaza nationalId duplicado. Habilita la validacion de remitente y
    // destinatario en registerPackage.
    bool registerClient(const Client& client);

    // --- Paquetes -----------------------------------------------------------
    // Rechaza codigo duplicado, peso <= 0, importe negativo, agencia destino
    // inexistente y cliente no registrado. Normaliza el pago:
    //   PREPAID           => PAID
    //   CASH_ON_DELIVERY  => PENDING
    // Un Package{} por defecto es un estado neutro previo al registro: no es
    // un paquete valido.
    bool registerPackage(const Package& pkg);

    std::optional<Package> trackPackage(const std::string& trackingCode) const;

    // CASH_ON_DELIVERY no puede llegar a DELIVERED mientras el pago siga
    // PENDING: esa transicion se rechaza aqui y en updatePackageStatus.
    bool updatePaymentStatus(const std::string& trackingCode,
                             PaymentStatus status);

    // Valida la transicion contra la tabla del SDD 5.1.
    //   AT_LOCAL_AGENCY -> OUT_FOR_DELIVERY  solo con HOME_DELIVERY
    //   OUT_FOR_DELIVERY -> DELIVERED        exige pago valido (COD => PAID)
    //   REGISTERED -> RETURNED               solo por cancelacion
    bool updatePackageStatus(const std::string& trackingCode,
                             PackageStatus status);

    // Solo desde REGISTERED.
    bool cancelPackage(const std::string& trackingCode);

    // --- Despacho -----------------------------------------------------------
    // Extrae el paquete mas urgente de la agencia (min-heap por ServiceLevel y
    // desempate FIFO por createdAt), actualiza el estado y el historial, y
    // devuelve la copia. nullopt si la agencia no existe o no tiene paquetes
    // elegibles.
    std::optional<Package> dispatchNextPackage(const std::string& agencyId);

    // --- Consultas y reportes (siempre copias) ------------------------------
    Designar::DynArray<Package> packagesAtAgency(const std::string& agencyId) const;
    Designar::DynArray<Package> packagesByStatus(PackageStatus status) const;

    // Suma de los importes cobrados, en centavos.
    std::int64_t totalRevenueCents() const;

    // --- Persistencia (SDD 6.6) --------------------------------------------
    // Formato de texto con std::quoted; load() limpia el estado antes de
    // reconstruirlo. Una linea corrupta devuelve false sin abortar.
    bool save(const std::string& path) const;
    bool load(const std::string& path);
};

} // namespace nodedispatch
