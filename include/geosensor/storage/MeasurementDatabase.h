#pragma once

#include "geosensor/data/SensorMeasurement.h"
#include "geosensor/data/SensorOrigin.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

struct sqlite3;

namespace geosensor::storage
{

// Stores radar measurements in a SQLite database.
class MeasurementDatabase
{
public:
    // Creates an empty database handle.
    MeasurementDatabase() = default;

    // Closes the database if it is open.
    ~MeasurementDatabase();

    MeasurementDatabase(const MeasurementDatabase&) = delete;
    MeasurementDatabase& operator=(const MeasurementDatabase&) = delete;

    // Transfers ownership of an open database connection.
    MeasurementDatabase(MeasurementDatabase&& other) noexcept;

    // Replaces this database connection with another owned connection.
    MeasurementDatabase& operator=(MeasurementDatabase&& other) noexcept;

    // Opens or creates the database file and required table.
    [[nodiscard]] bool open(const std::filesystem::path& databasePath);

    // Inserts one legacy live measurement row without a target identifier.
    [[nodiscard]] bool insertMeasurement(
        const data::SensorMeasurement& measurement
    );

    // Inserts one live measurement row with an optional target identifier.
    [[nodiscard]] bool insertTrackMeasurement(
        const data::SensorMeasurement& measurement,
        std::optional<std::int64_t> targetId,
        const std::chrono::system_clock::time_point& timestamp
    );

    // Deletes all stored measurement rows.
    [[nodiscard]] bool clearMeasurements();

    // Aggregates stored measurements by target identifier.
    struct TrackStatistics
    {
        std::int64_t targetId {};
        std::int64_t pointCount {};
        std::int64_t firstTimestampMs {};
        std::int64_t lastTimestampMs {};
        double minRangeM {};
        double maxRangeM {};
        double averageIntensity {};
    };

    // Returns aggregated per-target statistics from SQLite.
    [[nodiscard]] std::optional<std::vector<TrackStatistics>> trackStatistics() const;

    // Exports all stored measurement rows to a CSV file.
    [[nodiscard]] bool exportMeasurementsToCsv(
        const std::filesystem::path& csvPath
    );

    // Exports all stored measurement rows to a GeoJSON file.
    [[nodiscard]] bool exportMeasurementsToGeoJson(
        const std::filesystem::path& geoJsonPath,
        const data::SensorOrigin& sensorOrigin
    );

    // Exports tracked measurements as GeoJSON LineString features.
    [[nodiscard]] bool exportTracksToGeoJson(
        const std::filesystem::path& geoJsonPath,
        const data::SensorOrigin& sensorOrigin
    );

    // Returns the total stored measurement count.
    [[nodiscard]] std::optional<std::int64_t> measurementCount() const;

private:
    void close() noexcept;

    sqlite3* database_ {};
};

} // namespace geosensor::storage
