#pragma once

// -----------------------------------------------------------------------------
// NodeDispatch - DispatchCmp (S2.9)
//
// Comparador de la cola de despacho (SDD 5.3). Designar::DynHeap es un min-heap:
// `top()` devuelve el elemento que gana frente a todos los demas segun Cmp, asi
// que Cmp(a, b) == true significa "a sale antes que b".
//
// Orden de despacho:
//   1. ServiceLevel ascendente: 1 = EXPRESS_24H, 2 = PRIORITY, 3 = STANDARD.
//   2. Desempate FIFO por createdAt ascendente (el mas viejo primero).
//   3. Desempate estable por trackingCode. Sin el, dos paquetes del mismo nivel
//      con la misma marca de tiempo quedarian en un orden que depende de como
//      se insertaron, y el test no seria reproducible.
//
// No se define `operator<` en DispatchEntry a proposito: el orden de despacho es
// una regla de negocio y debe poder cambiar sin tocar el struct.
//
// Los valores 1/2/3 de ServiceLevel son un invariante: si alguien reordena el
// enum, el paquete mas urgente saldria al final. Los static_assert de
// tests/dispatch_cmp_test.cpp lo impiden.
// -----------------------------------------------------------------------------

#include "nodedispatch/package.hpp"

namespace nodedispatch {

struct DispatchCmp {
    bool operator()(const DispatchEntry& a, const DispatchEntry& b) const;
};

} // namespace nodedispatch
