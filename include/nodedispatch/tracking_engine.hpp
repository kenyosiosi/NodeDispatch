#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - TrackingEngine (S2.5)
//
// Fuente unica de verdad de los paquetes: Designar::HashMap<std::string, Package>
// indexado por trackingCode (SDD 6.2). El inventario y la cola de despacho
// guardan solo codigos; el estado real se consulta aqui.
//
//   * Alta: O(1) promedio
//   * Busqueda: O(1) promedio
//   * Cambio de estado: O(1) promedio
//
// Ninguna consulta devuelve punteros ni referencias a memoria interna: `find`
// entrega una copia, igual que `track` en el SDD 6.2. Un puntero al valor del
// mapa quedaria colgando con el primer rehash (que DeSiGNAR hace solo al pasar
// de alpha = 0.75).
//
// El motor no consulta el reloj: `updatedAt` lo fija quien llama (la fachada),
// para que las pruebas sean deterministas.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

#include <array.hpp>
#include <map.hpp>

#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class TrackingEngine {
public:
    TrackingEngine() = default;
    TrackingEngine(const TrackingEngine&) = delete;
    TrackingEngine& operator=(const TrackingEngine&) = delete;

    // --- Alta y consulta ------------------------------------------------------
    // Rechaza codigo vacio (INVALID_INPUT) y codigo repetido (DUPLICATE).
    OperationResult add(const Package& package);

    // Copia segura del paquete, o nullopt si no existe.
    std::optional<Package> find(const std::string& trackingCode) const;

    bool contains(const std::string& trackingCode) const;

    // --- Cambios de estado ----------------------------------------------------
    // updateStatus aplica la tabla del SDD 5.1 y la regla de entrega del 5.2:
    // las transiciones no listadas se rechazan con INVALID_STATE, y DELIVERED
    // exige pago valido cuando el tipo es CASH_ON_DELIVERY.
    OperationResult updateStatus(const std::string& trackingCode,
                                 PackageStatus status);

    // El SDD no restringe las transiciones de pago mas alla de la regla de
    // entrega; aqui solo se impide tocar un paquete ya entregado o devuelto.
    // Repetir el valor actual es idempotente (OK).
    OperationResult updatePaymentStatus(const std::string& trackingCode,
                                        PaymentStatus status);

    // --- Mantenimiento --------------------------------------------------------
    bool remove(const std::string& trackingCode);

    std::size_t size() const;

    void clear();

    // Paquetes con ese estado, en orden de insercion del mapa (no ordenado).
    // La fachada ordena o filtra lo que necesite (S3.8).
    Designar::DynArray<Package> byStatus(PackageStatus status) const;

private:
    Designar::HashMap<std::string, Package> index_;
};

// true si la transicion from -> to esta permitida por el SDD 5.1, con
// independencia del pago. Se expone para que la fachada y las pruebas usen la
// misma tabla en vez de duplicarla.
bool isTransitionAllowed(PackageStatus from, PackageStatus to);

} // namespace nodedispatch
