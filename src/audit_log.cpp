// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de AuditLog (S2.10)
// -----------------------------------------------------------------------------

#include "nodedispatch/audit_log.hpp"

namespace nodedispatch {

OperationResult AuditLog::append(const PackageEvent& event)
{
    if (event.trackingCode.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    events_.append(event); // al final: O(1)
    return OperationResult::OK;
}

Designar::DynArray<PackageEvent> AuditLog::history(
    const std::string& trackingCode) const
{
    Designar::DynArray<PackageEvent> out;

    if (trackingCode.empty())
    {
        return out;
    }

    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        if (it->trackingCode == trackingCode)
        {
            out.append(*it);
        }
    }

    return out;
}

std::size_t AuditLog::count(const std::string& trackingCode) const
{
    std::size_t total = 0;

    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        if (it->trackingCode == trackingCode)
        {
            ++total;
        }
    }

    return total;
}

std::size_t AuditLog::size() const
{
    return static_cast<std::size_t>(events_.size());
}

void AuditLog::clear()
{
    events_.clear();
}

} // namespace nodedispatch
