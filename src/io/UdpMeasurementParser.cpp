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

bool parseInteger(std::string_view field, int& value)
{
    const std::string trimmedField(trim(field));
    if (trimmedField.empty()) {
        return false;
    }

    std::size_t parsedLength = 0;

    try {
        value = std::stoi(trimmedField, &parsedLength);
    } catch (...) {
        return false;
    }

    return parsedLength == trimmedField.size();
}

} // namespace

namespace geosensor::io
{

std::optional<UdpMeasurementPacket> UdpMeasurementParser::parsePacket(
    std::string_view payload
)
{
    if (trim(payload).empty()) {
        return std::nullopt;
    }

    std::array<std::string_view, 5> fields;
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

    if (fieldIndex != 4 && fieldIndex != 5) {
        return std::nullopt;
    }

    UdpMeasurementPacket packet;
    std::size_t measurementFieldOffset = 0;

    if (fieldIndex == 5) {
        int targetId = 0;
        if (!parseInteger(fields[0], targetId)) {
            return std::nullopt;
        }

        packet.targetId = targetId;
        measurementFieldOffset = 1;
    }

    if (!parseDouble(fields[measurementFieldOffset], packet.measurement.rangeM)) {
        return std::nullopt;
    }
    if (!parseDouble(
            fields[measurementFieldOffset + 1],
            packet.measurement.azimuthDeg
        )) {
        return std::nullopt;
    }
    if (!parseDouble(
            fields[measurementFieldOffset + 2],
            packet.measurement.elevationDeg
        )) {
        return std::nullopt;
    }
    if (!parseDouble(
            fields[measurementFieldOffset + 3],
            packet.measurement.intensity
        )) {
        return std::nullopt;
    }

    return packet;
}

std::optional<data::SensorMeasurement> UdpMeasurementParser::parse(
    std::string_view payload
)
{
    const auto packet = parsePacket(payload);
    if (!packet.has_value()) {
        return std::nullopt;
    }

    return packet->measurement;
}

} // namespace geosensor::io
