#include "geosensor/coordinates/CoordinateTransform.h"
#include "geosensor/storage/MeasurementDatabase.h"

#include <sqlite3.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <locale>
#include <sstream>
#include <vector>

#ifdef GEOSENSOR_HAVE_GDAL
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_core.h>
#endif

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

std::string readCsvFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

[[maybe_unused]] std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

[[maybe_unused]] std::string formatJsonDouble(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(17) << value;
    return stream.str();
}

bool approximatelyEqual(double lhs, double rhs, double epsilon = 1e-9)
{
    return std::abs(lhs - rhs) <= epsilon;
}

#ifdef GEOSENSOR_HAVE_GDAL
struct GeoJsonFeatureExpectation
{
    std::optional<std::int64_t> targetId {};
    std::int64_t timestampMs {};
    geosensor::data::SensorMeasurement measurement {};
};

void validateGeoJsonExportWithGdal(
    const std::filesystem::path& geoJsonPath,
    const geosensor::data::SensorOrigin& sensorOrigin,
    const std::vector<GeoJsonFeatureExpectation>& expectations
)
{
    GDALAllRegister();

    GDALDatasetH dataset = GDALOpenEx(
        geoJsonPath.string().c_str(),
        GDAL_OF_VECTOR | GDAL_OF_READONLY,
        nullptr,
        nullptr,
        nullptr
    );

    assert(dataset != nullptr);

    OGRLayerH layer = GDALDatasetGetLayer(dataset, 0);
    assert(layer != nullptr);

    assert(OGR_L_GetFeatureCount(layer, 1) == static_cast<GIntBig>(expectations.size()));

    geosensor::coordinates::CoordinateTransform transform(sensorOrigin);
    OGR_L_ResetReading(layer);

    for (const auto& expectation : expectations) {
        OGRFeatureH feature = OGR_L_GetNextFeature(layer);
        assert(feature != nullptr);

        const int targetIdFieldIndex = OGR_F_GetFieldIndex(feature, "target_id");
        const int timestampFieldIndex = OGR_F_GetFieldIndex(feature, "timestamp_ms");
        const int rangeFieldIndex = OGR_F_GetFieldIndex(feature, "range_m");
        const int azimuthFieldIndex = OGR_F_GetFieldIndex(feature, "azimuth_deg");
        const int elevationFieldIndex = OGR_F_GetFieldIndex(feature, "elevation_deg");
        const int intensityFieldIndex = OGR_F_GetFieldIndex(feature, "intensity");

        assert(targetIdFieldIndex >= 0);
        assert(timestampFieldIndex >= 0);
        assert(rangeFieldIndex >= 0);
        assert(azimuthFieldIndex >= 0);
        assert(elevationFieldIndex >= 0);
        assert(intensityFieldIndex >= 0);

        if (expectation.targetId.has_value()) {
            assert(OGR_F_IsFieldSetAndNotNull(feature, targetIdFieldIndex));
            assert(
                OGR_F_GetFieldAsInteger64(feature, targetIdFieldIndex) ==
                *expectation.targetId
            );
        } else {
            assert(!OGR_F_IsFieldSetAndNotNull(feature, targetIdFieldIndex));
        }

        assert(
            OGR_F_GetFieldAsInteger64(feature, timestampFieldIndex) ==
            expectation.timestampMs
        );
        assert(
            approximatelyEqual(
                OGR_F_GetFieldAsDouble(feature, rangeFieldIndex),
                expectation.measurement.rangeM
            )
        );
        assert(
            approximatelyEqual(
                OGR_F_GetFieldAsDouble(feature, azimuthFieldIndex),
                expectation.measurement.azimuthDeg
            )
        );
        assert(
            approximatelyEqual(
                OGR_F_GetFieldAsDouble(feature, elevationFieldIndex),
                expectation.measurement.elevationDeg
            )
        );
        assert(
            approximatelyEqual(
                OGR_F_GetFieldAsDouble(feature, intensityFieldIndex),
                expectation.measurement.intensity
            )
        );

        OGRGeometryH geometry = OGR_F_GetGeometryRef(feature);
        assert(geometry != nullptr);

        const auto expectedPosition = transform.transform(expectation.measurement);
        assert(
            approximatelyEqual(
                OGR_G_GetX(geometry, 0),
                expectedPosition.geographic.longitudeDeg,
                1e-8
            )
        );
        assert(
            approximatelyEqual(
                OGR_G_GetY(geometry, 0),
                expectedPosition.geographic.latitudeDeg,
                1e-8
            )
        );

        OGR_F_Destroy(feature);
    }

    assert(OGR_L_GetNextFeature(layer) == nullptr);
    GDALClose(dataset);
}
#endif

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

void testClearMeasurementsEmptiesAnOpenDatabase()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 1200.0,
        .azimuthDeg = 45.0,
        .elevationDeg = 3.0,
        .intensity = 0.82
    };

    assert(database.insertMeasurement(measurement));
    assert(database.insertTrackMeasurement(
        measurement,
        7,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'123LL}
        }
    ));

    const auto countBeforeClear = database.measurementCount();
    assert(countBeforeClear.has_value());
    assert(*countBeforeClear == 2);

    assert(database.clearMeasurements());

    const auto countAfterClear = database.measurementCount();
    assert(countAfterClear.has_value());
    assert(*countAfterClear == 0);

    const auto rows = readStoredMeasurementRows();
    assert(rows.empty());
}

void testExportMeasurementsWritesCsv()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

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

    assert(database.insertTrackMeasurement(
        firstMeasurement,
        std::nullopt,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'123LL}
        }
    ));
    assert(database.insertTrackMeasurement(
        secondMeasurement,
        7,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'456LL}
        }
    ));

    const std::filesystem::path csvPath =
        std::filesystem::temp_directory_path() /
        "geosensor_measurement_database_tests_export.csv";
    std::error_code errorCode;
    std::filesystem::remove(csvPath, errorCode);

    assert(database.exportMeasurementsToCsv(csvPath));

    const std::string csvText = readCsvFile(csvPath);
    const std::string expectedCsv =
        "target_id,timestamp_ms,range_m,azimuth_deg,elevation_deg,intensity\n"
        ",1700000000123,1200,45,3,0.82\n"
        "7,1700000000456,950,70,1.5,0.64\n";

    assert(csvText == expectedCsv);

    std::filesystem::remove(csvPath, errorCode);
}

void testExportMeasurementsWritesGeoJson()
{
    removeTestDatabase();

    geosensor::storage::MeasurementDatabase database;
    assert(database.open(testDatabasePath()));

    const geosensor::data::SensorOrigin sensorOrigin {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };

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

    assert(database.insertTrackMeasurement(
        firstMeasurement,
        std::nullopt,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'123LL}
        }
    ));
    assert(database.insertTrackMeasurement(
        secondMeasurement,
        7,
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{1'700'000'000'456LL}
        }
    ));

    const std::filesystem::path geoJsonPath =
        std::filesystem::temp_directory_path() /
        "geosensor_measurement_database_tests_export.geojson";
    std::error_code errorCode;
    std::filesystem::remove(geoJsonPath, errorCode);

    assert(database.exportMeasurementsToGeoJson(geoJsonPath, sensorOrigin));

#ifdef GEOSENSOR_HAVE_GDAL
    assert(std::filesystem::exists(geoJsonPath));
    validateGeoJsonExportWithGdal(
        geoJsonPath,
        sensorOrigin,
        {
            GeoJsonFeatureExpectation {
                .targetId = std::nullopt,
                .timestampMs = 1'700'000'000'123LL,
                .measurement = firstMeasurement,
            },
            GeoJsonFeatureExpectation {
                .targetId = 7,
                .timestampMs = 1'700'000'000'456LL,
                .measurement = secondMeasurement,
            },
        }
    );
#else
    const geosensor::coordinates::CoordinateTransform transform(sensorOrigin);
    const auto firstPosition = transform.transform(firstMeasurement);
    const auto secondPosition = transform.transform(secondMeasurement);

    std::ostringstream expected;
    expected.imbue(std::locale::classic());
    expected
        << "{\n"
        << "  \"type\": \"FeatureCollection\",\n"
        << "  \"features\": [\n"
        << "    {\n"
        << "      \"type\": \"Feature\",\n"
        << "      \"geometry\": {\n"
        << "        \"type\": \"Point\",\n"
        << "        \"coordinates\": ["
        << formatJsonDouble(firstPosition.geographic.longitudeDeg) << ", "
        << formatJsonDouble(firstPosition.geographic.latitudeDeg) << "]\n"
        << "      },\n"
        << "      \"properties\": {\n"
        << "        \"target_id\": null,\n"
        << "        \"timestamp_ms\": 1700000000123,\n"
        << "        \"range_m\": " << formatJsonDouble(firstMeasurement.rangeM) << ",\n"
        << "        \"azimuth_deg\": " << formatJsonDouble(firstMeasurement.azimuthDeg) << ",\n"
        << "        \"elevation_deg\": " << formatJsonDouble(firstMeasurement.elevationDeg) << ",\n"
        << "        \"intensity\": " << formatJsonDouble(firstMeasurement.intensity) << "\n"
        << "      }\n"
        << "    },\n"
        << "    {\n"
        << "      \"type\": \"Feature\",\n"
        << "      \"geometry\": {\n"
        << "        \"type\": \"Point\",\n"
        << "        \"coordinates\": ["
        << formatJsonDouble(secondPosition.geographic.longitudeDeg) << ", "
        << formatJsonDouble(secondPosition.geographic.latitudeDeg) << "]\n"
        << "      },\n"
        << "      \"properties\": {\n"
        << "        \"target_id\": 7,\n"
        << "        \"timestamp_ms\": 1700000000456,\n"
        << "        \"range_m\": " << formatJsonDouble(secondMeasurement.rangeM) << ",\n"
        << "        \"azimuth_deg\": " << formatJsonDouble(secondMeasurement.azimuthDeg) << ",\n"
        << "        \"elevation_deg\": " << formatJsonDouble(secondMeasurement.elevationDeg) << ",\n"
        << "        \"intensity\": " << formatJsonDouble(secondMeasurement.intensity) << "\n"
        << "      }\n"
        << "    }\n"
        << "  ]\n"
        << "}\n";

    assert(readTextFile(geoJsonPath) == expected.str());
#endif

    std::filesystem::remove(geoJsonPath, errorCode);
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
    assert(!database.clearMeasurements());
    assert(!database.exportMeasurementsToCsv(
        std::filesystem::temp_directory_path() / "geosensor_measurement_database_tests_export.csv"
    ));
    assert(!database.exportMeasurementsToGeoJson(
        std::filesystem::temp_directory_path() / "geosensor_measurement_database_tests_export.geojson",
        geosensor::data::SensorOrigin {
            .latitudeDeg = 49.2488,
            .longitudeDeg = -122.9805,
            .altitudeM = 50.0
        }
    ));
    assert(!database.measurementCount().has_value());
}

} // namespace

int main()
{
    testOpenMigratesLegacySchemaAndStoresTrackAwareRows();
    testOpenCreatesFreshDatabaseAndStoresTrackAwareRows();
    testClearMeasurementsEmptiesAnOpenDatabase();
    testExportMeasurementsWritesCsv();
    testExportMeasurementsWritesGeoJson();
    testInsertFailsWhenDatabaseIsNotOpen();
    removeTestDatabase();

    std::cout << "All measurement database tests passed.\n";

    return 0;
}
