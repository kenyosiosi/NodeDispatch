#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - GeographicHierarchy
//
// Jerarquia N-aria de agencias (pais -> estado -> ciudad -> acopio) construida
// sobre Designar::MTreeNode<Agency>, con un indice Designar::HashMap para
// localizar cualquier nodo por su ID en O(1) promedio.
//
//   * Inserción: O(1)  (append_child + insert en el hash)
//   * Búsqueda por ID: O(1) promedio
//   * Descendientes (DFS) y recorrido por niveles (BFS): O(n)
//
// La jerarquía es de solo inserción: MTreeNode únicamente expone
// remove_first_child/remove_last_child, por lo que no se elimina un hijo
// intermedio. `clear()` destruye el árbol completo liberando toda la memoria.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

#include <array.hpp>
#include <map.hpp>
#include <mtreenode.hpp>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class GeographicHierarchy {
public:
    using Node = Designar::MTreeNode<Agency>;

    GeographicHierarchy() = default;
    ~GeographicHierarchy();

    GeographicHierarchy(const GeographicHierarchy&) = delete;
    GeographicHierarchy& operator=(const GeographicHierarchy&) = delete;

    // --- Mutacion -----------------------------------------------------------
    // `parentId` vacio crea la raiz (solo puede existir una).
    // Devuelve DUPLICATE, NOT_FOUND (padre inexistente), INVALID_STATE
    // (segunda raiz) o INVALID_INPUT (ID vacio).
    OperationResult addAgency(const Agency& agency,
                              const std::string& parentId = "");

    // Elimina todo el arbol y vacia el indice.
    void clear();

    // --- Consultas (siempre devuelven copias, nunca punteros internos) ------
    std::optional<Agency> findAgency(const std::string& id) const;
    bool contains(const std::string& id) const;

    std::optional<Agency> root() const;
    std::optional<Agency> parentOf(const std::string& id) const;

    // Profundidad de la agencia: raiz = 0. nullopt si no existe.
    std::optional<std::size_t> depthOf(const std::string& id) const;

    // Descendientes en preorden DFS (el nodo consultado no se incluye).
    Designar::DynArray<Agency> descendants(const std::string& id) const;

    // Recorrido por niveles BFS desde `id` (incluye el nodo consultado).
    Designar::DynArray<Agency> levelOrder(const std::string& id) const;

    // Numero de agencias registradas.
    std::size_t size() const;

private:
    // Devuelve el nodo o nullptr. Es const: solo se usa para leer.
    Node* findNode(const std::string& id) const;

    Node* root_ = nullptr;
    Designar::HashMap<std::string, Node*> index_;
};

} // namespace nodedispatch
