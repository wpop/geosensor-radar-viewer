#pragma once

#include "geosensor/data/SensorMeasurement.h"

#include <optional>
#include <string_view>

namespace geosensor::io
{

class UdpMeasurementParser
{
public:
    [[nodiscard]] static std::optional<data::SensorMeasurement> parse(
        std::string_view payload
    );
};

} // namespace geosensor::io
