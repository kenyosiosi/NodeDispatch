// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de PriorityDispatchQueue (S2.7)
// -----------------------------------------------------------------------------

#include "nodedispatch/dispatch_queue.hpp"

namespace nodedispatch {

bool isStaleEntry(const DispatchEntry& entry, const TrackingEngine& engine)
{
    const std::optional<Package> package = engine.find(entry.trackingCode);

    if (!package.has_value())
    {
        return true; // el paquete ya no existe
    }

    // Solo se despacha lo que esta fisicamente en la agencia de la cola.
    if (package->status != PackageStatus::AT_LOCAL_AGENCY)
    {
        return true;
    }

    if (package->targetAgencyId != entry.agencyId)
    {
        return true; // el paquete apunta a otra agencia
    }

    // Cambio de nivel de servicio o de marca de admision: la entrada vieja
    // sigue en el heap (DynHeap no permite actualizarla) y ya no vale.
    return package->serviceLevel != entry.level ||
           package->createdAt != entry.createdAt;
}

OperationResult PriorityDispatchQueue::enqueue(const DispatchEntry& entry)
{
    if (entry.trackingCode.empty() || entry.agencyId.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    // search_or_insert crea el heap de la agencia la primera vez.
    Queue* queue = index_.search_or_insert(entry.agencyId, Queue());
    if (queue == nullptr)
    {
        return OperationResult::INVALID_STATE; // no deberia ocurrir
    }

    queue->insert(entry);
    return OperationResult::OK;
}

std::optional<DispatchEntry> PriorityDispatchQueue::dispatch(
    const std::string& agencyId, const TrackingEngine& engine)
{
    Queue* queue = index_.search(agencyId); // version no const del mapa
    if (queue == nullptr)
    {
        return std::nullopt;
    }

    while (!queue->is_empty())
    {
        if (isStaleEntry(queue->top(), engine))
        {
            queue->get(); // se descarta y se sigue con el siguiente
            continue;
        }

        // top() devuelve una referencia: se copia antes de extraer.
        DispatchEntry next = queue->get();
        return next;
    }

    return std::nullopt;
}

std::size_t PriorityDispatchQueue::size(const std::string& agencyId) const
{
    const Queue* queue = findQueue(agencyId);
    return queue == nullptr ? 0U : static_cast<std::size_t>(queue->size());
}

std::size_t PriorityDispatchQueue::agencyCount() const
{
    return static_cast<std::size_t>(index_.size());
}

bool PriorityDispatchQueue::is_empty(const std::string& agencyId) const
{
    return size(agencyId) == 0;
}

void PriorityDispatchQueue::clear()
{
    index_.clear();
}

const PriorityDispatchQueue::Queue* PriorityDispatchQueue::findQueue(
    const std::string& agencyId) const
{
    return index_.search(agencyId);
}

} // namespace nodedispatch
