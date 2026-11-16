// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de DispatchCmp (S2.9)
// -----------------------------------------------------------------------------

#include "nodedispatch/dispatch_cmp.hpp"

namespace nodedispatch {

bool DispatchCmp::operator()(const DispatchEntry& a, const DispatchEntry& b) const
{
    // 1. Nivel de servicio: menor numero, mas urgente.
    if (a.level != b.level)
    {
        return static_cast<int>(a.level) < static_cast<int>(b.level);
    }

    // 2. FIFO: el que llego antes sale antes.
    if (a.createdAt != b.createdAt)
    {
        return a.createdAt < b.createdAt;
    }

    // 3. Desempate estable y total.
    return a.trackingCode < b.trackingCode;
}

} // namespace nodedispatch
