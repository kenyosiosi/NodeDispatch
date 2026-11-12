// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de GeographicHierarchy (S2.1)
// -----------------------------------------------------------------------------

#include "nodedispatch/geographic_hierarchy.hpp"

#include <functional>
#include <utility>

#include <queue.hpp>

namespace nodedispatch {

namespace {

// Preorden DFS: el nodo ya fue agregado por el llamador; aqui se agregan sus
// descendientes en el orden en que fueron insertados.
void collectPreorder(GeographicHierarchy::Node* node,
                     Designar::DynArray<Agency>& out)
{
    node->for_each_child([&](GeographicHierarchy::Node* child) {
        out.append(child->get_key());
        collectPreorder(child, out);
    });
}

} // namespace

GeographicHierarchy::~GeographicHierarchy()
{
    clear();
}

OperationResult GeographicHierarchy::addAgency(const Agency& agency,
                                               const std::string& parentId)
{
    if (agency.id.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    if (index_.has(agency.id))
    {
        return OperationResult::DUPLICATE;
    }

    Node* parent = nullptr;

    if (parentId.empty())
    {
        if (root_ != nullptr)
        {
            return OperationResult::INVALID_STATE; // ya existe una raiz
        }
    }
    else
    {
        parent = findNode(parentId);

        if (parent == nullptr)
        {
            return OperationResult::NOT_FOUND;
        }
    }

    auto* node = new Node(agency);

    // Se normaliza el padre para que el dato almacenado siempre coincida con la
    // posicion real dentro del arbol.
    node->get_key().parentId = parentId;

    if (parent == nullptr)
    {
        root_ = node;
    }
    else
    {
        parent->append_child(node);
    }

    if (index_.insert(node->get_key().id, node) == nullptr)
    {
        // No deberia ocurrir (el ID se valido arriba), pero se revierte para no
        // dejar un nodo en el arbol fuera del indice.
        if (parent != nullptr)
        {
            node = parent->remove_last_child();
        }
        else
        {
            root_ = nullptr;
        }

        Node::destroy_tree(node);
        return OperationResult::DUPLICATE;
    }

    return OperationResult::OK;
}

void GeographicHierarchy::clear()
{
    Node::destroy_tree(root_); // deja root_ == nullptr
    index_.clear();
}

GeographicHierarchy::Node* GeographicHierarchy::findNode(
    const std::string& id) const
{
    Node* const* found = index_.search(id);
    return found == nullptr ? nullptr : *found;
}

std::optional<Agency> GeographicHierarchy::findAgency(
    const std::string& id) const
{
    Node* node = findNode(id);

    if (node == nullptr)
    {
        return std::nullopt;
    }

    return node->get_key();
}

bool GeographicHierarchy::contains(const std::string& id) const
{
    return findNode(id) != nullptr;
}

std::optional<Agency> GeographicHierarchy::root() const
{
    if (root_ == nullptr)
    {
        return std::nullopt;
    }

    return root_->get_key();
}

std::optional<Agency> GeographicHierarchy::parentOf(const std::string& id) const
{
    Node* node = findNode(id);

    if (node == nullptr || node->get_parent() == nullptr)
    {
        return std::nullopt;
    }

    return node->get_parent()->get_key();
}

std::optional<std::size_t> GeographicHierarchy::depthOf(
    const std::string& id) const
{
    Node* node = findNode(id);

    if (node == nullptr)
    {
        return std::nullopt;
    }

    std::size_t depth = 0;

    for (Node* current = node->get_parent(); current != nullptr;
         current = current->get_parent())
    {
        ++depth;
    }

    return depth;
}

Designar::DynArray<Agency> GeographicHierarchy::descendants(
    const std::string& id) const
{
    Designar::DynArray<Agency> result;
    Node* start = findNode(id);

    if (start == nullptr)
    {
        return result;
    }

    collectPreorder(start, result);
    return result;
}

Designar::DynArray<Agency> GeographicHierarchy::levelOrder(
    const std::string& id) const
{
    Designar::DynArray<Agency> result;
    Node* start = findNode(id);

    if (start == nullptr)
    {
        return result;
    }

    Designar::ListQueue<Node*> pending;
    pending.put(start);

    while (!pending.is_empty())
    {
        Node* current = pending.get();
        result.append(current->get_key());

        current->for_each_child(
            [&](Node* child) { pending.put(child); });
    }

    return result;
}

std::size_t GeographicHierarchy::size() const
{
    return static_cast<std::size_t>(index_.size());
}

} // namespace nodedispatch
