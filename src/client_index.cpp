// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de ClientIndex (N1)
// -----------------------------------------------------------------------------

#include "nodedispatch/client_index.hpp"

namespace nodedispatch {

OperationResult ClientIndex::add(const Client& client)
{
    if (client.nationalId.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    // HashMap::insert devuelve nullptr si la clave ya existe: no reemplaza.
    if (index_.insert(client.nationalId, client) == nullptr)
    {
        return OperationResult::DUPLICATE;
    }

    return OperationResult::OK;
}

std::optional<Client> ClientIndex::find(const std::string& nationalId) const
{
    if (nationalId.empty())
    {
        return std::nullopt;
    }

    const Client* found = index_.search(nationalId);
    if (found == nullptr)
    {
        return std::nullopt;
    }

    return *found; // copia, no puntero interno
}

bool ClientIndex::contains(const std::string& nationalId) const
{
    return !nationalId.empty() && index_.search(nationalId) != nullptr;
}

bool ClientIndex::remove(const std::string& nationalId)
{
    return !nationalId.empty() && index_.remove(nationalId);
}

std::size_t ClientIndex::size() const
{
    return static_cast<std::size_t>(index_.size());
}

void ClientIndex::clear()
{
    index_.clear();
}

} // namespace nodedispatch
