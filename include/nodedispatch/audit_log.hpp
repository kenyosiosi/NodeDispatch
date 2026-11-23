#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - AuditLog (S2.10)
//
// Historial de eventos de los paquetes sobre Designar::DLList<PackageEvent>
// (SDD 6.5). Inserción al final O(1); recorrido O(n).
//
// El SDD deja abierta la lista por paquete "si se requiere consulta rápida".
// Aquí no se duplica: se guarda UNA lista global y `history()` filtra por
// código. Con el volumen de la demo (miles de eventos) filtrar es más barato
// que mantener dos estructuras sincronizadas, y un índice por paquete es
// material de los benchmarks de S4.4, no del MVP.
//
// El log no valida transiciones ni conoce el estado del paquete: solo registra
// lo que la fachada decide que ocurrió.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <string>

#include <array.hpp>
#include <list.hpp>

#include "nodedispatch/package.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class AuditLog {
public:
    AuditLog() = default;
    AuditLog(const AuditLog&) = delete;
    AuditLog& operator=(const AuditLog&) = delete;

    // Rechaza un evento sin codigo (INVALID_INPUT). El evento se copia.
    OperationResult append(const PackageEvent& event);

    // Eventos del paquete, del mas antiguo al mas reciente.
    Designar::DynArray<PackageEvent> history(const std::string& trackingCode) const;

    // Numero de eventos de ese paquete.
    std::size_t count(const std::string& trackingCode) const;

    std::size_t size() const;

    void clear();

private:
    Designar::DLList<PackageEvent> events_;
};

} // namespace nodedispatch
