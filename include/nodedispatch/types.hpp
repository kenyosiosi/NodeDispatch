#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Tipos base del dominio
//
// Enumeraciones y tipos de valor compartidos por todo el nucleo. Se usan
// `enum class` para evitar conversiones implicitas y enteros "magicos".
// -----------------------------------------------------------------------------

#include <cstdint>
#include <string>

namespace nodedispatch {

// --- Finanzas ---------------------------------------------------------------
enum class PaymentType {
    PREPAID,           // Pagado en origen por el emisor
    CASH_ON_DELIVERY   // Cobro destino
};

enum class PaymentStatus {
    PENDING,   // Pendiente de cobro
    PAID,      // Pagado y verificado
    FAILED     // Rechazado o fallido
};

// --- Entrega ----------------------------------------------------------------
enum class DeliveryMethod {
    AGENCY_PICKUP,   // Retiro en agencia
    HOME_DELIVERY    // Ultima milla (aplica recargo)
};

// Los valores numericos de ServiceLevel son invariantes: la cola de despacho
// compara niveles con DispatchCmp (SDD 5.3), que ordena de menor a mayor. Si
// EXPRESS_24H dejara de ser 1, el paquete mas urgente saldria al final sin que
// el comparador cambie. Ver SDD 4.1 "Enumeraciones".
enum class ServiceLevel {
    EXPRESS_24H = 1,
    PRIORITY    = 2,
    STANDARD    = 3
};

// --- Estado operativo -------------------------------------------------------
enum class PackageStatus {
    REGISTERED,        // Registrado en el sistema
    IN_TRANSIT,        // En traslado entre nodos
    AT_LOCAL_AGENCY,   // Disponible en la agencia destino
    OUT_FOR_DELIVERY,  // En reparto (solo HOME_DELIVERY)
    DELIVERED,         // Entregado
    RETURNED           // Devuelto
};

enum class ReturnReason {
    NONE,
    PAYMENT_FAILED,
    RECIPIENT_ABSENT,
    CANCELLED
};

// --- Resultado de operaciones de la fachada ---------------------------------
// La API publica no lanza excepciones de negocio: devuelve este codigo.
enum class OperationResult {
    OK,
    NOT_FOUND,
    DUPLICATE,
    INVALID_STATE,
    INVALID_INPUT
};

// --- Tipos de valor ---------------------------------------------------------
struct Address {
    std::string state;
    std::string city;
    std::string exactAddress;
    std::string postalCode;
};

} // namespace nodedispatch
