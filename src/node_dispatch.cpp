// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion de la fachada (N3)
//
// La fachada orquesta los modulos; no reimplementa sus reglas. Cada validacion
// que ya vive en un modulo se delega, para que la tabla de transiciones o la
// normalizacion del pago existan una sola vez.
// -----------------------------------------------------------------------------

#include "nodedispatch/node_dispatch.hpp"

#include <ctime>
#include <string>

#include "nodedispatch/agency_api.hpp"

namespace nodedispatch {

namespace {

std::int64_t nowSeconds()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

DispatchEntry makeEntry(const Package& package)
{
    DispatchEntry entry;
    entry.trackingCode = package.trackingCode;
    entry.agencyId = package.targetAgencyId;
    entry.level = package.serviceLevel;
    entry.createdAt = package.createdAt;
    return entry;
}

void logEvent(AuditLog& log, const std::string& trackingCode, PackageStatus status,
              const std::string& note)
{
    PackageEvent event;
    event.trackingCode = trackingCode;
    event.status = status;
    event.at = nowSeconds();
    event.note = note;
    log.append(event);
}

const char* paymentStatusName(PaymentStatus status)
{
    switch (status)
    {
    case PaymentStatus::PENDING:
        return "PENDING";
    case PaymentStatus::PAID:
        return "PAID";
    case PaymentStatus::FAILED:
        return "FAILED";
    }
    return "DESCONOCIDO";
}

} // namespace

// --- Jerarquia ---------------------------------------------------------------
// La traduccion al contrato publico vive en AgencyApi (S3.7), no aqui.

bool NodeDispatch::addAgency(const Agency& agency, const std::string& parentId)
{
    AgencyApi api(agencies_);
    return api.addAgency(agency, parentId);
}

std::optional<Agency> NodeDispatch::findAgency(const std::string& id) const
{
    const AgencyApi api(agencies_);
    return api.findAgency(id);
}

Designar::DynArray<Agency> NodeDispatch::listDescendants(const std::string& id) const
{
    const AgencyApi api(agencies_);
    return api.listDescendants(id);
}

// --- Clientes ----------------------------------------------------------------

bool NodeDispatch::registerClient(const Client& client)
{
    return clients_.add(client) == OperationResult::OK;
}

// --- Paquetes ----------------------------------------------------------------

bool NodeDispatch::registerPackage(const Package& pkg)
{
    Package package = pkg;

    // Codigo vacio: se genera uno con el formato ND-AAAAMMDD-NNNNNN (N2).
    if (package.trackingCode.empty())
    {
        if (codes_.next(nowSeconds(), package.trackingCode) != OperationResult::OK)
        {
            return false;
        }
    }

    if (package.weightKg <= 0.0)
    {
        return false;
    }

    if (package.baseCents < 0 || package.deliverySurchargeCents < 0)
    {
        return false;
    }

    if (package.senderId.empty() || package.receiverId.empty() ||
        package.targetAgencyId.empty())
    {
        return false;
    }

    if (!clients_.contains(package.senderId) ||
        !clients_.contains(package.receiverId))
    {
        return false;
    }

    if (!agencies_.findAgency(package.targetAgencyId).has_value())
    {
        return false;
    }

    // Regla del SDD 5.2: el tipo de pago fija el estado inicial del cobro.
    package.paymentStatus = package.paymentType == PaymentType::PREPAID
                                ? PaymentStatus::PAID
                                : PaymentStatus::PENDING;

    if (package.createdAt == 0)
    {
        package.createdAt = nowSeconds();
    }
    package.updatedAt = package.createdAt;

    if (packages_.add(package) != OperationResult::OK)
    {
        return false; // codigo duplicado
    }

    // Un paquete puede registrarse ya en la agencia destino (previo traslado
    // manual): entonces entra en el inventario y en la cola de despacho.
    if (package.status == PackageStatus::AT_LOCAL_AGENCY)
    {
        inventory_.add(package.targetAgencyId, package.trackingCode);
        queues_.enqueue(makeEntry(package));
    }

    logEvent(audit_, package.trackingCode, package.status, "Alta en el sistema");
    return true;
}

std::optional<Package> NodeDispatch::trackPackage(const std::string& trackingCode) const
{
    return packages_.find(trackingCode);
}

bool NodeDispatch::updatePaymentStatus(const std::string& trackingCode,
                                       PaymentStatus status)
{
    if (packages_.updatePaymentStatus(trackingCode, status) != OperationResult::OK)
    {
        return false;
    }

    const std::optional<Package> package = packages_.find(trackingCode);
    if (!package.has_value())
    {
        return false;
    }

    logEvent(audit_, trackingCode, package->status,
             std::string("Pago: ") + paymentStatusName(status));
    return true;
}

bool NodeDispatch::updatePackageStatus(const std::string& trackingCode,
                                       PackageStatus status)
{
    if (packages_.updateStatus(trackingCode, status) != OperationResult::OK)
    {
        return false;
    }

    const std::optional<Package> package = packages_.find(trackingCode);
    if (!package.has_value())
    {
        return false;
    }

    // Inventario y cola siguen al estado; el paquete sigue siendo del engine.
    if (status == PackageStatus::AT_LOCAL_AGENCY)
    {
        inventory_.add(package->targetAgencyId, trackingCode);
        queues_.enqueue(makeEntry(*package));
    }
    else if (status != PackageStatus::IN_TRANSIT && status != PackageStatus::REGISTERED)
    {
        inventory_.remove(package->targetAgencyId, trackingCode);
    }

    logEvent(audit_, trackingCode, status, "Cambio de estado");
    return true;
}

bool NodeDispatch::cancelPackage(const std::string& trackingCode)
{
    // La tabla 5.1 solo permite salir de REGISTERED; el engine lo valida.
    return updatePackageStatus(trackingCode, PackageStatus::RETURNED);
}

// --- Despacho ----------------------------------------------------------------

std::optional<Package> NodeDispatch::dispatchNextPackage(const std::string& agencyId)
{
    if (agencyId.empty())
    {
        return std::nullopt;
    }

    // Los paquetes que no se pueden despachar (por ejemplo, Cobro Destino sin
    // pagar) se apartan y se devuelven a la cola al final, para que no bloqueen
    // a los que si pueden salir.
    Designar::DynArray<DispatchEntry> skipped;
    const std::size_t attempts = queues_.size(agencyId);

    std::optional<Package> dispatched;

    for (std::size_t i = 0; i < attempts; ++i)
    {
        const std::optional<DispatchEntry> entry = queues_.dispatch(agencyId, packages_);
        if (!entry.has_value())
        {
            break;
        }

        const std::optional<Package> package = packages_.find(entry->trackingCode);
        if (!package.has_value())
        {
            continue; // el engine ya no lo tiene: la entrada se descarta
        }

        const PackageStatus next =
            package->deliveryMethod == DeliveryMethod::HOME_DELIVERY
                ? PackageStatus::OUT_FOR_DELIVERY
                : PackageStatus::DELIVERED;

        if (packages_.updateStatus(entry->trackingCode, next) != OperationResult::OK)
        {
            skipped.append(*entry); // sigue elegible cuando se cobre
            continue;
        }

        inventory_.remove(agencyId, entry->trackingCode);
        logEvent(audit_, entry->trackingCode, next,
                 next == PackageStatus::OUT_FOR_DELIVERY ? "Despacho a reparto"
                                                         : "Entregado en agencia");

        dispatched = packages_.find(entry->trackingCode);
        break;
    }

    for (std::size_t i = 0; i < skipped.size(); ++i)
    {
        queues_.enqueue(skipped[i]);
    }

    return dispatched;
}

// --- Consultas ---------------------------------------------------------------

Designar::DynArray<Package> NodeDispatch::packagesAtAgency(
    const std::string& agencyId) const
{
    Designar::DynArray<Package> out;

    // El inventario guarda los codigos que estan fisicamente en la agencia.
    const Designar::DynArray<std::string> codes = inventory_.listSorted(agencyId);
    for (std::size_t i = 0; i < codes.size(); ++i)
    {
        const std::optional<Package> package = packages_.find(codes[i]);
        if (package.has_value())
        {
            out.append(*package);
        }
    }

    return out;
}

Designar::DynArray<Package> NodeDispatch::packagesByStatus(PackageStatus status) const
{
    return packages_.byStatus(status);
}

std::int64_t NodeDispatch::totalRevenueCents() const
{
    std::int64_t total = 0;

    const Designar::DynArray<Package> all = packages_.all();
    for (std::size_t i = 0; i < all.size(); ++i)
    {
        const Package& package = all[i];
        if (package.paymentStatus != PaymentStatus::PAID)
        {
            continue; // solo se cobra lo efectivamente pagado
        }

        total += totalCents(package);
    }

    return total;
}

// --- Persistencia (S4.2) -----------------------------------------------------

bool NodeDispatch::save(const std::string& path) const
{
    (void)path;
    return false; // S4.2: formato definido en docs/snapshot-format.md (S1.7)
}

bool NodeDispatch::load(const std::string& path)
{
    (void)path;
    return false; // S4.2
}

} // namespace nodedispatch
