#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - Generador de codigos de rastreo (N2)
//
// Formato: ND-AAAAMMDD-NNNNNN, en UTC.
//
//   * ND          prefijo fijo del proyecto
//   * AAAAMMDD    fecha de admision, para que el codigo sea legible y ordenable
//   * NNNNNN      contador del dia, seis digitos con ceros a la izquierda
//
// El contador se reinicia al cambiar de dia y admite 999999 codigos diarios.
// Si se agota, next() devuelve INVALID_STATE en vez de repetir un codigo: la
// unicidad del trackingCode es la clave del TrackingEngine.
//
// La conversion de fecha es aritmetica (dias desde la epoca), no usa gmtime:
// gmtime devuelve un puntero a un buffer estatico compartido y depende del TZ
// del proceso, dos cosas indeseables en una funcion que se llamara por cada
// paquete registrado.
// -----------------------------------------------------------------------------

#include <cstdint>
#include <string>

#include "nodedispatch/types.hpp"

namespace nodedispatch {

// Tope del contador diario.
inline constexpr std::uint64_t kMaxTrackingSequence = 999999;

// "ND-AAAAMMDD-NNNNNN" para esa marca de tiempo Unix (segundos UTC).
// Devuelve cadena vacia si unixSeconds es negativo o sequence supera el tope.
std::string formatTrackingCode(std::int64_t unixSeconds, std::uint64_t sequence);

// Contador diario. No es seguro entre hilos: la fachada lo protegera cuando
// haya concurrencia (S8, mejora opcional).
class TrackingCodeGenerator {
public:
    TrackingCodeGenerator() = default;

    // Escribe en `out` el siguiente codigo del dia de unixSeconds.
    // INVALID_INPUT si la marca es negativa; INVALID_STATE si el dia ya emitio
    // 999999 codigos.
    OperationResult next(std::int64_t unixSeconds, std::string& out);

    std::uint64_t sequence() const
    {
        return sequence_;
    }

private:
    std::int64_t day_ = -1;      // dia Unix (segundos / 86400)
    std::uint64_t sequence_ = 0; // codigos emitidos ese dia
};

} // namespace nodedispatch
