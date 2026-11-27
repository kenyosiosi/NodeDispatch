// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de TrackingEngine (S2.5)
// -----------------------------------------------------------------------------

#include "nodedispatch/tracking_engine.hpp"

namespace nodedispatch {

// La regla de pago vive ahora en canTransition(), junto a la tabla de estados:
// un solo sitio para las dos condiciones del SDD 5.2.

bool isTransitionAllowed(PackageStatus from, PackageStatus to)
{
    switch (from)
    {
    case PackageStatus::REGISTERED:
        return to == PackageStatus::IN_TRANSIT || to == PackageStatus::RETURNED;
    case PackageStatus::IN_TRANSIT:
        return to == PackageStatus::AT_LOCAL_AGENCY;
    case PackageStatus::AT_LOCAL_AGENCY:
        return to == PackageStatus::OUT_FOR_DELIVERY ||
               to == PackageStatus::DELIVERED || to == PackageStatus::RETURNED;
    case PackageStatus::OUT_FOR_DELIVERY:
        return to == PackageStatus::DELIVERED || to == PackageStatus::RETURNED;
    case PackageStatus::DELIVERED:
    case PackageStatus::RETURNED:
        return false; // estados terminales
    }
    return false;
}

bool canTransition(PackageStatus from, PackageStatus to,
                   PaymentStatus paymentStatus, DeliveryMethod deliveryMethod)
{
    if (!isTransitionAllowed(from, to))
    {
        return false;
    }

    if (to == PackageStatus::OUT_FOR_DELIVERY &&
        deliveryMethod != DeliveryMethod::HOME_DELIVERY)
    {
        return false;
    }

    if (to == PackageStatus::DELIVERED && paymentStatus != PaymentStatus::PAID)
    {
        return false;
    }

    return true;
}

OperationResult TrackingEngine::add(const Package& package)
{
    if (package.trackingCode.empty())
    {
        return OperationResult::INVALID_INPUT;
    }

    // HashMap::insert devuelve nullptr si la clave ya existe.
    if (index_.insert(package.trackingCode, package) == nullptr)
    {
        return OperationResult::DUPLICATE;
    }

    return OperationResult::OK;
}

std::optional<Package> TrackingEngine::find(const std::string& trackingCode) const
{
    if (trackingCode.empty())
    {
        return std::nullopt;
    }

    const Package* found = index_.search(trackingCode);
    if (found == nullptr)
    {
        return std::nullopt;
    }

    return *found; // copia
}

bool TrackingEngine::contains(const std::string& trackingCode) const
{
    return !trackingCode.empty() && index_.search(trackingCode) != nullptr;
}

OperationResult TrackingEngine::updateStatus(const std::string& trackingCode,
                                             PackageStatus status)
{
    Package* package = trackingCode.empty() ? nullptr : index_.search(trackingCode);
    if (package == nullptr)
    {
        return OperationResult::NOT_FOUND;
    }

    // Tabla 5.1 y reglas del 5.2 en un solo sitio.
    if (!canTransition(package->status, status, package->paymentStatus,
                       package->deliveryMethod))
    {
        return OperationResult::INVALID_STATE;
    }

    package->status = status;
    return OperationResult::OK;
}

OperationResult TrackingEngine::updatePaymentStatus(
    const std::string& trackingCode, PaymentStatus status)
{
    Package* package = trackingCode.empty() ? nullptr : index_.search(trackingCode);
    if (package == nullptr)
    {
        return OperationResult::NOT_FOUND;
    }

    if (package->status == PackageStatus::DELIVERED ||
        package->status == PackageStatus::RETURNED)
    {
        return OperationResult::INVALID_STATE; // paquete cerrado
    }

    package->paymentStatus = status;
    return OperationResult::OK;
}

bool TrackingEngine::remove(const std::string& trackingCode)
{
    return !trackingCode.empty() && index_.remove(trackingCode);
}

std::size_t TrackingEngine::size() const
{
    return static_cast<std::size_t>(index_.size());
}

void TrackingEngine::clear()
{
    index_.clear();
}

Designar::DynArray<Package> TrackingEngine::all() const
{
    Designar::DynArray<Package> out;
    for (auto it = index_.begin(); it != index_.end(); ++it)
    {
        out.append(Designar::value(*it));
    }
    return out;
}

Designar::DynArray<Package> TrackingEngine::byStatus(PackageStatus status) const
{
    Designar::DynArray<Package> out;
    for (auto it = index_.begin(); it != index_.end(); ++it)
    {
        const Package& package = Designar::value(*it);
        if (package.status == status)
        {
            out.append(package);
        }
    }
    return out;
}

} // namespace nodedispatch
