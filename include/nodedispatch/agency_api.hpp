#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - API de agencias (S3.7)
//
// Adaptador de la jerarquia para la fachada (SDD 7). Traduce el contrato de
// GeographicHierarchy al contrato publico de NodeDispatch:
//
//   addAgency        OperationResult -> bool
//   findAgency       copia segura en std::optional
//   listDescendants  preorden DFS (descendants), sin incluir el nodo
//
// Existe para que la fachada no repita la traduccion en cada metodo: los tres
// metodos de jerarquia de NodeDispatch delegan aqui (ver src/node_dispatch.cpp).
//
// Tiene dos constructores para respetar const-correctness: uno mutable, que
// habilita addAgency, y uno de solo lectura para findAgency/listDescendants en
// los metodos const de la fachada.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

#include <array.hpp>

#include "nodedispatch/agency.hpp"
#include "nodedispatch/geographic_hierarchy.hpp"
#include "nodedispatch/types.hpp"

namespace nodedispatch {

class AgencyApi {
public:
    explicit AgencyApi(GeographicHierarchy& hierarchy)
        : readable_(&hierarchy), writable_(&hierarchy)
    {
    }

    explicit AgencyApi(const GeographicHierarchy& hierarchy)
        : readable_(&hierarchy), writable_(nullptr)
    {
    }

    // `parentId` vacio crea la raiz. false si el ID esta vacio, ya existe o el
    // padre no existe; el detalle del rechazo lo decide la jerarquia.
    bool addAgency(const Agency& agency, const std::string& parentId = "")
    {
        if (writable_ == nullptr)
        {
            return false; // vista de solo lectura
        }

        return writable_->addAgency(agency, parentId) == OperationResult::OK;
    }

    // Copia de la agencia; nullopt si no existe.
    std::optional<Agency> findAgency(const std::string& id) const
    {
        return readable_->findAgency(id);
    }

    // Descendientes en preorden DFS. El nodo consultado no se incluye.
    Designar::DynArray<Agency> listDescendants(const std::string& id) const
    {
        return readable_->descendants(id);
    }

    // Numero de agencias registradas (reportes y presentacion).
    std::size_t size() const
    {
        return readable_->size();
    }

private:
    const GeographicHierarchy* readable_;
    GeographicHierarchy* writable_;
};

} // namespace nodedispatch
