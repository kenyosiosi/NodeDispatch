#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - ClientIndex (N1)
//
// Indice de clientes sobre Designar::HashMap<std::string, Client>. Guarda una
// sola copia por cliente: Package solo referencia senderId/receiverId, asi que
// cambiar el telefono de alguien no obliga a recorrer sus paquetes.
//
//   * Alta: O(1) promedio
//   * Busqueda: O(1) promedio
//   * Baja: O(1) promedio
//
// La clave es nationalId. No se valida su formato: el dominio no impone uno y
// la fachada puede aceptar cedula, RIF o pasaporte.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

#include <map.hpp>

#include "nodedispatch/client.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class ClientIndex {
public:
    ClientIndex() = default;
    ClientIndex(const ClientIndex&) = delete;
    ClientIndex& operator=(const ClientIndex&) = delete;

    // Rechaza nationalId vacio (INVALID_INPUT) y cliente repetido (DUPLICATE).
    // El cliente se copia: el indice es dueno de su propia copia.
    OperationResult add(const Client& client);

    // Copia segura: nunca devuelve un puntero al dato interno.
    std::optional<Client> find(const std::string& nationalId) const;

    bool contains(const std::string& nationalId) const;

    bool remove(const std::string& nationalId);

    std::size_t size() const;

    void clear();

private:
    Designar::HashMap<std::string, Client> index_;
};

} // namespace nodedispatch
