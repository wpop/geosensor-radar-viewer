#pragma once

#include "geosensor/data/SensorMeasurement.h"

#include <filesystem>
#include <vector>

namespace geosensor::io
{

// Loads radar-like sensor measurements from a CSV file.
//
// Expected CSV format:
// range_m,azimuth_deg,elevation_deg,intensity
//
// The first line is treated as a header and skipped.
class CsvMeasurementLoader
{
public:
    [[nodiscard]] std::vector<data::SensorMeasurement> load(
        const std::filesystem::path& filePath
    ) const;
};

} // namespace geosensor::io
