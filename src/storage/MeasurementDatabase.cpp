#include "geosensor/storage/MeasurementDatabase.h"

#include <sqlite3.h>

#include <string>

namespace
{

constexpr const char* kCreateMeasurementTableSql =
    "CREATE TABLE IF NOT EXISTS measurements ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "range_m REAL NOT NULL,"
    "azimuth_deg REAL NOT NULL,"
    "elevation_deg REAL NOT NULL,"
    "intensity REAL NOT NULL"
    ");";

constexpr const char* kInsertMeasurementSql =
    "INSERT INTO measurements ("
    "range_m, azimuth_deg, elevation_deg, intensity"
    ") VALUES (?, ?, ?, ?);";

constexpr const char* kMeasurementCountSql =
    "SELECT COUNT(*) FROM measurements;";

} // namespace

namespace geosensor::storage
{

MeasurementDatabase::~MeasurementDatabase()
{
    close();
}

MeasurementDatabase::MeasurementDatabase(MeasurementDatabase&& other) noexcept
    : database_(other.database_)
{
    other.database_ = nullptr;
}

MeasurementDatabase& MeasurementDatabase::operator=(
    MeasurementDatabase&& other
) noexcept
{
    if (this == &other) {
        return *this;
    }

    close();
    database_ = other.database_;
    other.database_ = nullptr;

    return *this;
}

bool MeasurementDatabase::open(const std::filesystem::path& databasePath)
{
    close();

    if (sqlite3_open(databasePath.string().c_str(), &database_) != SQLITE_OK) {
        close();
        return false;
    }

    if (
        sqlite3_exec(
            database_,
            kCreateMeasurementTableSql,
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK
    ) {
        close();
        return false;
    }

    return true;
}

bool MeasurementDatabase::insertMeasurement(
    const data::SensorMeasurement& measurement
)
{
    if (database_ == nullptr) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database_,
            kInsertMeasurementSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    sqlite3_bind_double(statement, 1, measurement.rangeM);
    sqlite3_bind_double(statement, 2, measurement.azimuthDeg);
    sqlite3_bind_double(statement, 3, measurement.elevationDeg);
    sqlite3_bind_double(statement, 4, measurement.intensity);

    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);

    return stepResult == SQLITE_DONE;
}

std::optional<std::int64_t> MeasurementDatabase::measurementCount() const
{
    if (database_ == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database_,
            kMeasurementCountSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return std::nullopt;
    }

    const int stepResult = sqlite3_step(statement);

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const std::int64_t count = sqlite3_column_int64(statement, 0);
    sqlite3_finalize(statement);

    return count;
}

void MeasurementDatabase::close() noexcept
{
    if (database_ != nullptr) {
        sqlite3_close(database_);
        database_ = nullptr;
    }
}

} // namespace geosensor::storage
