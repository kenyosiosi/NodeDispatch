#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Cliente
//
// Se normaliza en un ClientIndex (HashMap<string, Client>) para no duplicar
// datos dentro de cada Package. Package guarda solo senderId / receiverId.
// -----------------------------------------------------------------------------

#include <string>

#include "nodedispatch/types.hpp"

namespace nodedispatch {

struct Client {
    std::string nationalId;   // Clave unica
    std::string firstName;
    std::string lastName;
    std::string phone;
    Address address;
};

} // namespace nodedispatch
