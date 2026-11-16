// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de InventoryIndex (S2.3)
// -----------------------------------------------------------------------------

#include "nodedispatch/inventory_index.hpp"

namespace nodedispatch {

InventoryIndex::~InventoryIndex()
{
    clear();
}

OperationResult InventoryIndex::add(const std::string& agencyId,
                                    const std::string& trackingCode)
{
    if (agencyId.empty() || trackingCode.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    Tree* const* found = index_.search(agencyId);
    Tree* tree = found == nullptr ? nullptr : *found;

    if (tree == nullptr)
    {
        tree = new Tree();
        index_.insert(agencyId, tree);
    }

    // RbTree::insert devuelve nullptr si la clave ya existe (no reemplaza).
    if (tree->insert(trackingCode) == nullptr)
    {
        return OperationResult::DUPLICATE;
    }

    return OperationResult::OK;
}

bool InventoryIndex::remove(const std::string& agencyId,
                            const std::string& trackingCode)
{
    Tree* tree = findTree(agencyId);
    return tree != nullptr && tree->remove(trackingCode);
}

bool InventoryIndex::contains(const std::string& agencyId,
                              const std::string& trackingCode) const
{
    const Tree* tree = findTree(agencyId);
    return tree != nullptr && tree->search(trackingCode) != nullptr;
}

Designar::DynArray<std::string> InventoryIndex::listSorted(
    const std::string& agencyId) const
{
    Designar::DynArray<std::string> out;
    const Tree* tree = findTree(agencyId);

    if (tree == nullptr)
    {
        return out;
    }

    // begin()/end() recorren el arbol en in-order: el resultado ya sale
    // ascendente sin ordenar despues.
    for (auto it = tree->begin(); it != tree->end(); ++it)
    {
        out.append(*it);
    }

    return out;
}

std::size_t InventoryIndex::size(const std::string& agencyId) const
{
    const Tree* tree = findTree(agencyId);
    return tree == nullptr ? 0U : static_cast<std::size_t>(tree->size());
}

std::size_t InventoryIndex::agencyCount() const
{
    return static_cast<std::size_t>(index_.size());
}

bool InventoryIndex::is_empty(const std::string& agencyId) const
{
    return size(agencyId) == 0;
}

void InventoryIndex::clear()
{
    // El mapa indexa pero no posee: los arboles son de este objeto y hay que
    // liberarlos uno a uno antes de vaciar el indice.
    for (auto it = index_.begin(); it != index_.end(); ++it)
    {
        delete Designar::value(*it);
    }

    index_.clear();
}

InventoryIndex::Tree* InventoryIndex::findTree(
    const std::string& agencyId) const
{
    Tree* const* found = index_.search(agencyId);
    return found == nullptr ? nullptr : *found;
}

} // namespace nodedispatch
