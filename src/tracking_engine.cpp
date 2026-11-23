// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de TrackingEngine (S2.5)
// -----------------------------------------------------------------------------

#include "nodedispatch/tracking_engine.hpp"

namespace nodedispatch {

namespace {

// Regla del SDD 5.2: un paquete con Cobro Destino solo se entrega pagado. Los
// prepagados ya nacen PAID, asi que la condicion se cumple siempre.
bool paymentAllowsDelivery(const Package& package)
{
    return package.paymentType == PaymentType::PREPAID ||
           package.paymentStatus == PaymentStatus::PAID;
}

} // namespace

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

    if (!isTransitionAllowed(package->status, status))
    {
        return OperationResult::INVALID_STATE;
    }

    // Condiciones de la tabla 5.1.
    if (status == PackageStatus::OUT_FOR_DELIVERY &&
        package->deliveryMethod != DeliveryMethod::HOME_DELIVERY)
    {
        return OperationResult::INVALID_STATE;
    }

    if (status == PackageStatus::DELIVERED && !paymentAllowsDelivery(*package))
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
