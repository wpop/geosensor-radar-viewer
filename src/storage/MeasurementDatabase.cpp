#include "geosensor/storage/MeasurementDatabase.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace
{

constexpr const char* kCreateMeasurementTableSql =
    "CREATE TABLE IF NOT EXISTS measurements ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "target_id INTEGER,"
    "timestamp_ms INTEGER NOT NULL,"
    "range_m REAL NOT NULL,"
    "azimuth_deg REAL NOT NULL,"
    "elevation_deg REAL NOT NULL,"
    "intensity REAL NOT NULL"
    ");";

constexpr const char* kInsertMeasurementSql =
    "INSERT INTO measurements ("
    "target_id, timestamp_ms, range_m, azimuth_deg, elevation_deg, intensity"
    ") VALUES (?, ?, ?, ?, ?, ?);";

constexpr const char* kMeasurementCountSql =
    "SELECT COUNT(*) FROM measurements;";

constexpr const char* kClearMeasurementsSql =
    "DELETE FROM measurements;";

constexpr const char* kTargetIdColumnName = "target_id";
constexpr const char* kTimestampColumnName = "timestamp_ms";

bool columnExists(
    sqlite3* database,
    std::string_view tableName,
    std::string_view columnName
)
{
    sqlite3_stmt* statement = nullptr;

    const std::string pragmaSql = "PRAGMA table_info(" + std::string(tableName) + ");";
    if (
        sqlite3_prepare_v2(
            database,
            pragmaSql.c_str(),
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    bool exists = false;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char* nameText = sqlite3_column_text(statement, 1);
        if (
            nameText != nullptr &&
            columnName == reinterpret_cast<const char*>(nameText)
        ) {
            exists = true;
            break;
        }
    }

    sqlite3_finalize(statement);
    return exists;
}

bool addColumnIfMissing(
    sqlite3* database,
    std::string_view tableName,
    std::string_view columnName,
    std::string_view columnDefinition
)
{
    if (columnExists(database, tableName, columnName)) {
        return true;
    }

    const std::string alterSql =
        "ALTER TABLE " + std::string(tableName) +
        " ADD COLUMN " + std::string(columnDefinition) + ";";

    return sqlite3_exec(database, alterSql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool ensureMeasurementSchema(sqlite3* database)
{
    if (
        sqlite3_exec(
            database,
            kCreateMeasurementTableSql,
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    if (
        !addColumnIfMissing(
            database,
            "measurements",
            kTargetIdColumnName,
            "target_id INTEGER"
        )
    ) {
        return false;
    }

    return addColumnIfMissing(
        database,
        "measurements",
        kTimestampColumnName,
        "timestamp_ms INTEGER NOT NULL DEFAULT 0"
    );
}

bool insertMeasurementRow(
    sqlite3* database,
    const geosensor::data::SensorMeasurement& measurement,
    std::optional<std::int64_t> targetId,
    std::int64_t timestampMs
)
{
    if (database == nullptr) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database,
            kInsertMeasurementSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    if (targetId.has_value()) {
        sqlite3_bind_int64(statement, 1, *targetId);
    } else {
        sqlite3_bind_null(statement, 1);
    }
    sqlite3_bind_int64(statement, 2, timestampMs);
    sqlite3_bind_double(statement, 3, measurement.rangeM);
    sqlite3_bind_double(statement, 4, measurement.azimuthDeg);
    sqlite3_bind_double(statement, 5, measurement.elevationDeg);
    sqlite3_bind_double(statement, 6, measurement.intensity);

    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);

    return stepResult == SQLITE_DONE;
}

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

    if (!ensureMeasurementSchema(database_)) {
        close();
        return false;
    }

    return true;
}

bool MeasurementDatabase::insertMeasurement(
    const data::SensorMeasurement& measurement
)
{
    return insertTrackMeasurement(
        measurement,
        std::nullopt,
        std::chrono::system_clock::now()
    );
}

bool MeasurementDatabase::insertTrackMeasurement(
    const data::SensorMeasurement& measurement,
    std::optional<std::int64_t> targetId,
    const std::chrono::system_clock::time_point& timestamp
)
{
    const std::int64_t timestampMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()
        ).count();

    return insertMeasurementRow(
        database_,
        measurement,
        targetId,
        timestampMs
    );
}

bool MeasurementDatabase::clearMeasurements()
{
    if (database_ == nullptr) {
        return false;
    }

    return sqlite3_exec(
        database_,
        kClearMeasurementsSql,
        nullptr,
        nullptr,
        nullptr
    ) == SQLITE_OK;
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
