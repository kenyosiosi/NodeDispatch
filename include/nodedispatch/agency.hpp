#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Agencia
//
// Nodo de la jerarquia geografica. Se almacena dentro de un
// Designar::MTreeNode<Agency> y se indexa por `id` en un HashMap.
// -----------------------------------------------------------------------------

#include <string>

#include "nodedispatch/types.hpp"

namespace nodedispatch {

struct Agency {
    std::string id;        // Clave unica
    std::string name;
    Address location;
    std::string parentId;  // Vacio para la raiz
};

} // namespace nodedispatch
