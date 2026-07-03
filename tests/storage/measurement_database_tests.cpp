#include "geosensor/storage/MeasurementDatabase.h"

#include <sqlite3.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

namespace
{

struct StoredMeasurementRow
{
    std::optional<std::int64_t> targetId {};
    std::int64_t timestampMs {};
};

std::filesystem::path testDatabasePath()
{
    return
        std::filesystem::temp_directory_path() /
        "geosensor_measurement_database_tests.sqlite";
}

void removeTestDatabase()
{
    std::error_code errorCode;
    std::filesystem::remove(testDatabasePath(), errorCode);
}

void createLegacyMeasurementDatabase()
{
    sqlite3* database = nullptr;
    assert(sqlite3_open(testDatabasePath().string().c_str(), &database) == SQLITE_OK);

    const char* legacySchemaSql =
        "CREATE TABLE measurements ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "range_m REAL NOT NULL,"
        "azimuth_deg REAL NOT NULL,"
        "elevation_deg REAL NOT NULL,"
        "intensity REAL NOT NULL"
        ");";

    assert(
        sqlite3_exec(database, legacySchemaSql, nullptr, nullptr, nullptr) ==
        SQLITE_OK
    );
    assert(sqlite3_close(database) == SQLITE_OK);
}

std::vector<StoredMeasurementRow> readStoredMeasurementRows()
{
    sqlite3* database = nullptr;
    assert(sqlite3_open(testDatabasePath().string().c_str(), &database) == SQLITE_OK);

    sqlite3_stmt* statement = nullptr;
    assert(
        sqlite3_prepare_v2(
            database,
            "SELECT target_id, timestamp_ms FROM measurements ORDER BY id;",
            -1,
            &statement,
            nullptr
        ) == SQLITE_OK
    );

    std::vector<StoredMeasurementRow> rows;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        StoredMeasurementRow row;

        if (sqlite3_column_type(statement, 0) != SQLITE_NULL) {
            row.targetId = sqlite3_column_int64(statement, 0);
        }

        row.timestampMs = sqlite3_column_int64(statement, 1);
        rows.push_back(row);
    }

    sqlite3_finalize(statement);
    assert(sqlite3_close(database) == SQLITE_OK);

    return rows;
}

void testOpenMigratesLegacySchemaAndStoresTrackAwareRows()
{
    removeTestDatabase();
    createLegacyMeasurementDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

    const auto initialCount = database.measurementCount();
    assert(initialCount.has_value());
    assert(*initialCount == 0);

    const geosensor::data::SensorMeasurement legacyMeasurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    const geosensor::data::SensorMeasurement trackMeasurement {
        .rangeM = 950.0,
        .azimuthDeg = 70.0,
        .elevationDeg = 1.5,
        .intensity = 0.64
    };

    const auto fixedTimestamp =
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'123LL}
        };

    assert(database.insertMeasurement(legacyMeasurement));
    assert(database.insertTrackMeasurement(trackMeasurement, 7, fixedTimestamp));

    const auto count = database.measurementCount();
    assert(count.has_value());
    assert(*count == 2);

    const auto rows = readStoredMeasurementRows();
    assert(rows.size() == 2);
    assert(!rows[0].targetId.has_value());
    assert(rows[0].timestampMs > 0);
    assert(rows[1].targetId.has_value());
    assert(*rows[1].targetId == 7);
    assert(rows[1].timestampMs == 1'700'000'000'123LL);
}

void testOpenCreatesFreshDatabaseAndStoresTrackAwareRows()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

    const auto initialCount = database.measurementCount();
    assert(initialCount.has_value());
    assert(*initialCount == 0);

    const geosensor::data::SensorMeasurement legacyMeasurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    const geosensor::data::SensorMeasurement trackMeasurement {
        .rangeM = 950.0,
        .azimuthDeg = 70.0,
        .elevationDeg = 1.5,
        .intensity = 0.64
    };

    const auto fixedTimestamp =
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'123LL}
        };

    assert(database.insertMeasurement(legacyMeasurement));
    assert(database.insertTrackMeasurement(trackMeasurement, 7, fixedTimestamp));

    const auto count = database.measurementCount();
    assert(count.has_value());
    assert(*count == 2);

    const auto rows = readStoredMeasurementRows();
    assert(rows.size() == 2);
    assert(!rows[0].targetId.has_value());
    assert(rows[0].timestampMs > 0);
    assert(rows[1].targetId.has_value());
    assert(*rows[1].targetId == 7);
    assert(rows[1].timestampMs == 1'700'000'000'123LL);
}

void testInsertFailsWhenDatabaseIsNotOpen()
{
    geosensor::storage::MeasurementDatabase database;

    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    assert(!database.insertMeasurement(measurement));
    assert(!database.insertTrackMeasurement(
        measurement,
        7,
        std::chrono::system_clock::now()
    ));
    assert(!database.measurementCount().has_value());
}

} // namespace

int main()
{
    testOpenMigratesLegacySchemaAndStoresTrackAwareRows();
    testOpenCreatesFreshDatabaseAndStoresTrackAwareRows();
    testInsertFailsWhenDatabaseIsNotOpen();
    removeTestDatabase();

    std::cout << "All measurement database tests passed.\n";

    return 0;
}
