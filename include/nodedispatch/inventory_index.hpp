#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - InventoryIndex (S2.3)
//
// Inventario por agencia construido sobre Designar::RbTree<std::string>. Guarda
// SOLO codigos de rastreo: el Package vive en el TrackingEngine, que es la
// fuente unica de verdad. Duplicar el paquete aqui permitiria que un cambio de
// estado quedara aplicado en un sitio y desfasado en otro.
//
//   * Alta: O(log n) por agencia
//   * Baja: O(log n)
//   * Pertenencia: O(log n)
//   * Lista ordenada: O(n) en orden in-order
//
// La estructura es tolerante a agencias vacias: el arbol se crea en la primera
// insercion de esa agencia y se libera con clear(). No se valida que la agencia
// exista en la jerarquia; eso lo decide la fachada.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <string>

#include <array.hpp>
#include <map.hpp>
#include <rbtree.hpp>

#include "nodedispatch/types.hpp"

namespace nodedispatch {

class InventoryIndex {
public:
    using Tree = Designar::RbTree<std::string>;

    InventoryIndex() = default;
    ~InventoryIndex();

    InventoryIndex(const InventoryIndex&) = delete;
    InventoryIndex& operator=(const InventoryIndex&) = delete;

    // --- Mutacion -----------------------------------------------------------
    // Rechaza codigo vacio (INVALID_INPUT) y codigo repetido en la misma
    // agencia (DUPLICATE). El mismo codigo puede estar en dos agencias
    // distintas: el traslado entre nodos se modela con dos altas hasta que la
    // fachada decida retirar la anterior.
    OperationResult add(const std::string& agencyId,
                        const std::string& trackingCode);

    // Devuelve false si la agencia o el codigo no existen.
    bool remove(const std::string& agencyId, const std::string& trackingCode);

    // --- Consultas ----------------------------------------------------------
    bool contains(const std::string& agencyId,
                  const std::string& trackingCode) const;

    // Codigos de la agencia en orden ascendente (in-order del arbol).
    Designar::DynArray<std::string> listSorted(const std::string& agencyId) const;

    std::size_t size(const std::string& agencyId) const;

    // Numero de agencias con al menos un alta.
    std::size_t agencyCount() const;

    bool is_empty(const std::string& agencyId) const;

    // Libera todos los arboles.
    void clear();

private:
    // Arbol de la agencia o nullptr. Es const: solo se usa para leer.
    Tree* findTree(const std::string& agencyId) const;

    Designar::HashMap<std::string, Tree*> index_;
};

} // namespace nodedispatch
