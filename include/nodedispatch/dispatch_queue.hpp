#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - PriorityDispatchQueue (S2.7)
//
// Una cola por agencia: Designar::HashMap<std::string, Designar::DynHeap<
// DispatchEntry, DispatchCmp>> (SDD 6.4). La entrada guarda solo el codigo: el
// estado real vive en el TrackingEngine.
//
// DynHeap no permite borrar un elemento arbitrario ni cambiar su prioridad, asi
// que se usa **eliminacion perezosa**: al despachar se descartan las entradas
// que ya no describen al paquete.
//
//   * Encolar: O(log n)
//   * Despachar: O(log n) amortizado (cada entrada obsoleta se paga una vez)
//   * Consultar tamano: O(1)
//
// La cola NO cambia el estado del paquete ni escribe el historial: eso es de la
// fachada (SDD 7, dispatchNextPackage), que es quien conoce el reloj y el
// AuditLog. Aqui solo se elige el siguiente.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

#include <heap.hpp>
#include <map.hpp>

#include "nodedispatch/dispatch_cmp.hpp"
#include "nodedispatch/package.hpp"
#include "nodedispatch/tracking_engine.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class PriorityDispatchQueue {
public:
    using Queue = Designar::DynHeap<DispatchEntry, DispatchCmp>;

    PriorityDispatchQueue() = default;
    PriorityDispatchQueue(const PriorityDispatchQueue&) = delete;
    PriorityDispatchQueue& operator=(const PriorityDispatchQueue&) = delete;

    // Rechaza trackingCode o agencyId vacios (INVALID_INPUT). Encolar dos veces
    // el mismo codigo es valido a proposito: el cambio de nivel de servicio se
    // modela asi y la entrada vieja se descarta al despachar.
    OperationResult enqueue(const DispatchEntry& entry);

    // Siguiente paquete despachable de la agencia, o nullopt si no queda
    // ninguno. Descarta (sin devolver) las entradas cuyo paquete ya no esta
    // AT_LOCAL_AGENCY, cambio de agencia destino o cambio de nivel o de marca
    // de admision desde que se encolo.
    std::optional<DispatchEntry> dispatch(const std::string& agencyId,
                                          const TrackingEngine& engine);

    // Entradas encoladas, incluidas las que aun no se han descartado.
    std::size_t size(const std::string& agencyId) const;

    std::size_t agencyCount() const;

    bool is_empty(const std::string& agencyId) const;

    void clear();

private:
    const Queue* findQueue(const std::string& agencyId) const;

    Designar::HashMap<std::string, Queue> index_;
};

// true si la entrada ya no describe al paquete (o el paquete desaparecio).
// Se expone para que la fachada pueda auditar el mismo criterio.
bool isStaleEntry(const DispatchEntry& entry, const TrackingEngine& engine);

} // namespace nodedispatch
