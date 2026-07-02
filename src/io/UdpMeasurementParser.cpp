#include "geosensor/io/UdpMeasurementParser.h"

#include <array>
#include <cctype>
#include <string>

namespace
{

std::string_view trim(std::string_view text) noexcept
{
    std::size_t begin = 0;
    while (
        begin < text.size() &&
        std::isspace(static_cast<unsigned char>(text[begin])) != 0
    ) {
        ++begin;
    }

    std::size_t end = text.size();
    while (
        end > begin &&
        std::isspace(static_cast<unsigned char>(text[end - 1])) != 0
    ) {
        --end;
    }

    return text.substr(begin, end - begin);
}

bool parseDouble(std::string_view field, double& value)
{
    const std::string trimmedField(trim(field));
    if (trimmedField.empty()) {
        return false;
    }

    std::size_t parsedLength = 0;

    try {
        value = std::stod(trimmedField, &parsedLength);
    } catch (...) {
        return false;
    }

    return parsedLength == trimmedField.size();
}

} // namespace

namespace geosensor::io
{

std::optional<data::SensorMeasurement> UdpMeasurementParser::parse(
    std::string_view payload
)
{
    if (trim(payload).empty()) {
        return std::nullopt;
    }

    std::array<std::string_view, 4> fields;
    std::size_t fieldIndex = 0;
    std::size_t fieldStart = 0;

    for (std::size_t i = 0; i <= payload.size(); ++i) {
        if (i != payload.size() && payload[i] != ',') {
            continue;
        }

        if (fieldIndex >= fields.size()) {
            return std::nullopt;
        }

        fields[fieldIndex] = payload.substr(fieldStart, i - fieldStart);
        ++fieldIndex;
        fieldStart = i + 1;
    }

    if (fieldIndex != fields.size()) {
        return std::nullopt;
    }

    data::SensorMeasurement measurement;

    if (!parseDouble(fields[0], measurement.rangeM)) {
        return std::nullopt;
    }
    if (!parseDouble(fields[1], measurement.azimuthDeg)) {
        return std::nullopt;
    }
    if (!parseDouble(fields[2], measurement.elevationDeg)) {
        return std::nullopt;
    }
    if (!parseDouble(fields[3], measurement.intensity)) {
        return std::nullopt;
    }

    return measurement;
}

} // namespace geosensor::io
