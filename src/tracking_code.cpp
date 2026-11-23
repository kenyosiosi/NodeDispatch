// -----------------------------------------------------------------------------
// NodeDispatch - Implementacion del generador de codigos (N2)
// -----------------------------------------------------------------------------

#include "nodedispatch/tracking_code.hpp"

#include <cstddef>

namespace nodedispatch {

namespace {

constexpr std::int64_t kSecondsPerDay = 86400;

// Fecha civil a partir de dias desde 1970-01-01 (algoritmo de Howard Hinnant,
// valido para todo el rango de int64 que nos interesa). Evita gmtime y su
// buffer estatico compartido.
void civilFromDays(std::int64_t days, int& year, unsigned& month, unsigned& day)
{
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t y = static_cast<std::int64_t>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year = static_cast<int>(y + (month <= 2 ? 1 : 0));
}

std::string pad(std::uint64_t value, std::size_t width)
{
    std::string digits = std::to_string(value);
    if (digits.size() >= width)
    {
        return digits;
    }
    return std::string(width - digits.size(), '0') + digits;
}

} // namespace

std::string formatTrackingCode(std::int64_t unixSeconds, std::uint64_t sequence)
{
    if (unixSeconds < 0 || sequence > kMaxTrackingSequence)
    {
        return std::string();
    }

    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    civilFromDays(unixSeconds / kSecondsPerDay, year, month, day);

    std::string code = "ND-";
    code += pad(static_cast<std::uint64_t>(year), 4);
    code += pad(month, 2);
    code += pad(day, 2);
    code += '-';
    code += pad(sequence, 6);
    return code;
}

OperationResult TrackingCodeGenerator::next(std::int64_t unixSeconds,
                                            std::string& out)
{
    if (unixSeconds < 0)
    {
        return OperationResult::INVALID_INPUT;
    }

    const std::int64_t currentDay = unixSeconds / kSecondsPerDay;
    if (currentDay != day_)
    {
        day_ = currentDay;
        sequence_ = 0;
    }

    if (sequence_ >= kMaxTrackingSequence)
    {
        return OperationResult::INVALID_STATE;
    }

    ++sequence_;
    out = formatTrackingCode(unixSeconds, sequence_);
    return OperationResult::OK;
}

} // namespace nodedispatch
