#pragma once

#include "geosensor/data/SensorMeasurement.h"

#include <optional>
#include <string_view>

namespace geosensor::io
{

// One UDP measurement packet with an optional target identifier.
struct UdpMeasurementPacket
{
    std::optional<int> targetId {};
    data::SensorMeasurement measurement {};
};

// Parses UDP CSV payloads into measurement data.
class UdpMeasurementParser
{
public:
    // Parses a UDP payload in 4-field or 5-field CSV format.
    [[nodiscard]] static std::optional<UdpMeasurementPacket> parsePacket(
        std::string_view payload
    );

    // Parses a UDP payload and returns only the measurement values.
    [[nodiscard]] static std::optional<data::SensorMeasurement> parse(
        std::string_view payload
    );
};

} // namespace geosensor::io
