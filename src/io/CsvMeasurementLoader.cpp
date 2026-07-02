#include "geosensor/io/CsvMeasurementLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{

geosensor::data::SensorMeasurement parseMeasurementLine(
    const std::string& line
)
{
    std::stringstream stream(line);
    std::string field;

    geosensor::data::SensorMeasurement measurement;

    if (!std::getline(stream, field, ',')) {
        throw std::runtime_error("Missing range field");
    }
    measurement.rangeM = std::stod(field);

    if (!std::getline(stream, field, ',')) {
        throw std::runtime_error("Missing azimuth field");
    }
    measurement.azimuthDeg = std::stod(field);

    if (!std::getline(stream, field, ',')) {
        throw std::runtime_error("Missing elevation field");
    }
    measurement.elevationDeg = std::stod(field);

    if (!std::getline(stream, field, ',')) {
        throw std::runtime_error("Missing intensity field");
    }
    measurement.intensity = std::stod(field);

    return measurement;
}

} // namespace

namespace geosensor::io
{

std::vector<data::SensorMeasurement> CsvMeasurementLoader::load(
    const std::filesystem::path& filePath
) const
{
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open CSV file: " + filePath.string()
        );
    }

    std::vector<data::SensorMeasurement> measurements;
    std::string line;

    // Skip header.
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        measurements.push_back(parseMeasurementLine(line));
    }

    return measurements;
}

} // namespace geosensor::io
