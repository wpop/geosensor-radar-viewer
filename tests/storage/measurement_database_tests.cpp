#include "geosensor/storage/MeasurementDatabase.h"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace
{

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

void testOpenCreatesDatabase()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));
    assert(std::filesystem::exists(testDatabasePath()));
}

void testInsertAndCountMeasurements()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

    const auto initialCount = database.measurementCount();
    assert(initialCount.has_value());
    assert(*initialCount == 0);

    const geosensor::data::SensorMeasurement firstMeasurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    const geosensor::data::SensorMeasurement secondMeasurement {
        .rangeM = 950.0,
        .azimuthDeg = 70.0,
        .elevationDeg = 1.5,
        .intensity = 0.64
    };

    assert(database.insertMeasurement(firstMeasurement));
    assert(database.insertMeasurement(secondMeasurement));

    const auto count = database.measurementCount();
    assert(count.has_value());
    assert(*count == 2);
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
    assert(!database.measurementCount().has_value());
}

} // namespace

int main()
{
    testOpenCreatesDatabase();
    testInsertAndCountMeasurements();
    testInsertFailsWhenDatabaseIsNotOpen();
    removeTestDatabase();

    std::cout << "All measurement database tests passed.\n";

    return 0;
}
