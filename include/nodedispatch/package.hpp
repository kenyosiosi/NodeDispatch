#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Paquete y estructuras asociadas
//
// Package es la entidad canonica y vive en el TrackingEngine
// (HashMap<string, Package>). El inventario y la cola de despacho guardan solo
// el codigo de rastreo, nunca una copia completa.
//
// El dinero se maneja en centavos enteros (std::int64_t) para evitar errores de
// punto flotante al comparar o acumular importes.
// -----------------------------------------------------------------------------

#include <cstdint>
#include <string>

#include "nodedispatch/types.hpp"

namespace nodedispatch {

struct Package {
    std::string trackingCode;    // Clave unica de rastreo
    std::string senderId;        // Referencia a ClientIndex
    std::string receiverId;      // Referencia a ClientIndex
    std::string targetAgencyId;  // Agencia destino

    double weightKg = 0.0;

    std::int64_t baseCents = 0;               // Costo base en centavos
    std::int64_t deliverySurchargeCents = 0;  // Recargo de delivery en centavos

    PaymentType paymentType = PaymentType::PREPAID;
    PaymentStatus paymentStatus = PaymentStatus::PENDING;

    DeliveryMethod deliveryMethod = DeliveryMethod::AGENCY_PICKUP;
    ServiceLevel serviceLevel = ServiceLevel::STANDARD;
    PackageStatus status = PackageStatus::REGISTERED;
    ReturnReason returnReason = ReturnReason::NONE;

    std::int64_t createdAt = 0;  // Marca de tiempo Unix de admision
    std::int64_t updatedAt = 0;  // Ultima modificacion
};

// Entrada de la cola de prioridad. Guarda solo el codigo: el estado real se
// consulta en el TrackingEngine (eliminacion perezosa de entradas obsoletas).
struct DispatchEntry {
    std::string trackingCode;
    std::string agencyId;
    ServiceLevel level = ServiceLevel::STANDARD;
    std::int64_t createdAt = 0;
};

// Evento del historial de un paquete (AuditLog con Designar::DLList).
struct PackageEvent {
    std::string trackingCode;
    PackageStatus status = PackageStatus::REGISTERED;
    std::int64_t at = 0;
    std::string note;
};

} // namespace nodedispatch
