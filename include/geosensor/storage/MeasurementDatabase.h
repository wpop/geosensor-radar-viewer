#pragma once

#include "geosensor/data/SensorMeasurement.h"

#include <cstdint>
#include <filesystem>
#include <optional>

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

    // Inserts one measurement row into the database.
    [[nodiscard]] bool insertMeasurement(
        const data::SensorMeasurement& measurement
    );

    // Returns the total stored measurement count.
    [[nodiscard]] std::optional<std::int64_t> measurementCount() const;

private:
    void close() noexcept;

    sqlite3* database_ {};
};

} // namespace geosensor::storage
