#include "geosensor/storage/MeasurementDatabase.h"

#include "geosensor/coordinates/CoordinateTransform.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string>
#include <string_view>
#include <vector>

#ifdef GEOSENSOR_HAVE_GDAL
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_core.h>
#include <ogr_srs_api.h>
#endif

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

constexpr const char* kTrackStatisticsSql =
    "SELECT "
    "target_id, "
    "COUNT(*), "
    "MIN(timestamp_ms), "
    "MAX(timestamp_ms), "
    "MIN(range_m), "
    "MAX(range_m), "
    "AVG(intensity) "
    "FROM measurements "
    "WHERE target_id IS NOT NULL "
    "GROUP BY target_id "
    "ORDER BY target_id;";

constexpr const char* kExportMeasurementsSql =
    "SELECT target_id, timestamp_ms, range_m, azimuth_deg, elevation_deg, intensity "
    "FROM measurements ORDER BY id;";

#ifdef GEOSENSOR_HAVE_GDAL
constexpr const char* kExportTracksSql =
    "SELECT target_id, timestamp_ms, range_m, azimuth_deg, elevation_deg, intensity "
    "FROM measurements "
    "WHERE target_id IS NOT NULL "
    "ORDER BY target_id, timestamp_ms, id;";
#endif

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

[[maybe_unused]] void writeJsonDouble(std::ostream& output, double value)
{
    output << std::setprecision(17) << value;
}

#ifdef GEOSENSOR_HAVE_GDAL
bool addGdalField(
    OGRLayerH layer,
    const char* fieldName,
    OGRFieldType fieldType
)
{
    OGRFieldDefnH fieldDefinition = OGR_Fld_Create(fieldName, fieldType);
    if (fieldDefinition == nullptr) {
        return false;
    }

    OGR_Fld_SetNullable(fieldDefinition, TRUE);

    const OGRErr createFieldResult = OGR_L_CreateField(layer, fieldDefinition, TRUE);
    OGR_Fld_Destroy(fieldDefinition);

    return createFieldResult == OGRERR_NONE;
}

bool exportMeasurementsToGeoJsonWithGdal(
    sqlite3* database,
    const std::filesystem::path& geoJsonPath,
    const geosensor::data::SensorOrigin& sensorOrigin
)
{
    std::error_code errorCode;
    std::filesystem::remove(geoJsonPath, errorCode);

    GDALAllRegister();

    OGRSFDriverH driver = OGRGetDriverByName("GeoJSON");
    if (driver == nullptr) {
        return false;
    }

    OGRDataSourceH dataSource =
        OGR_Dr_CreateDataSource(driver, geoJsonPath.string().c_str(), nullptr);
    if (dataSource == nullptr) {
        return false;
    }

    OGRSpatialReferenceH spatialReference = OSRNewSpatialReference(nullptr);
    if (spatialReference == nullptr) {
        OGR_DS_Destroy(dataSource);
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        OSRImportFromEPSG(spatialReference, 4326) != OGRERR_NONE
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    OSRSetAxisMappingStrategy(spatialReference, OAMS_TRADITIONAL_GIS_ORDER);

    OGRLayerH layer = OGR_DS_CreateLayer(
        dataSource,
        "measurements",
        spatialReference,
        wkbPoint,
        nullptr
    );
    if (layer == nullptr) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    if (
        !addGdalField(layer, "target_id", OFTInteger64) ||
        !addGdalField(layer, "timestamp_ms", OFTInteger64) ||
        !addGdalField(layer, "range_m", OFTReal) ||
        !addGdalField(layer, "azimuth_deg", OFTReal) ||
        !addGdalField(layer, "elevation_deg", OFTReal) ||
        !addGdalField(layer, "intensity", OFTReal)
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    const geosensor::coordinates::CoordinateTransform transform(sensorOrigin);

    if (
        sqlite3_prepare_v2(
            database,
            kExportMeasurementsSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    const OGRFeatureDefnH layerDefinition = OGR_L_GetLayerDefn(layer);
    const int targetIdFieldIndex = OGR_FD_GetFieldIndex(layerDefinition, "target_id");
    const int timestampFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "timestamp_ms");
    const int rangeFieldIndex = OGR_FD_GetFieldIndex(layerDefinition, "range_m");
    const int azimuthFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "azimuth_deg");
    const int elevationFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "elevation_deg");
    const int intensityFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "intensity");

    if (
        targetIdFieldIndex < 0 ||
        timestampFieldIndex < 0 ||
        rangeFieldIndex < 0 ||
        azimuthFieldIndex < 0 ||
        elevationFieldIndex < 0 ||
        intensityFieldIndex < 0
    ) {
        sqlite3_finalize(statement);
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    int stepResult = SQLITE_ROW;

    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        const auto measurement = geosensor::data::SensorMeasurement {
            .rangeM = sqlite3_column_double(statement, 2),
            .azimuthDeg = sqlite3_column_double(statement, 3),
            .elevationDeg = sqlite3_column_double(statement, 4),
            .intensity = sqlite3_column_double(statement, 5),
        };
        const auto targetPosition = transform.transform(measurement);

        OGRFeatureH feature = OGR_F_Create(layerDefinition);
        if (feature == nullptr) {
            sqlite3_finalize(statement);
            OSRDestroySpatialReference(spatialReference);
            OGR_DS_Destroy(dataSource);
            return false;
        }

        if (sqlite3_column_type(statement, 0) == SQLITE_NULL) {
            OGR_F_SetFieldNull(feature, targetIdFieldIndex);
        } else {
            OGR_F_SetFieldInteger64(
                feature,
                targetIdFieldIndex,
                sqlite3_column_int64(statement, 0)
            );
        }

        OGR_F_SetFieldInteger64(
            feature,
            timestampFieldIndex,
            sqlite3_column_int64(statement, 1)
        );
        OGR_F_SetFieldDouble(feature, rangeFieldIndex, measurement.rangeM);
        OGR_F_SetFieldDouble(feature, azimuthFieldIndex, measurement.azimuthDeg);
        OGR_F_SetFieldDouble(
            feature,
            elevationFieldIndex,
            measurement.elevationDeg
        );
        OGR_F_SetFieldDouble(feature, intensityFieldIndex, measurement.intensity);

        OGRGeometryH pointGeometry = OGR_G_CreateGeometry(wkbPoint);
        if (pointGeometry == nullptr) {
            OGR_F_Destroy(feature);
            sqlite3_finalize(statement);
            OSRDestroySpatialReference(spatialReference);
            OGR_DS_Destroy(dataSource);
            return false;
        }

        OGR_G_SetPoint_2D(
            pointGeometry,
            0,
            targetPosition.geographic.longitudeDeg,
            targetPosition.geographic.latitudeDeg
        );

        if (OGR_F_SetGeometryDirectly(feature, pointGeometry) != OGRERR_NONE) {
            OGR_F_Destroy(feature);
            sqlite3_finalize(statement);
            OSRDestroySpatialReference(spatialReference);
            OGR_DS_Destroy(dataSource);
            return false;
        }

        if (OGR_L_CreateFeature(layer, feature) != OGRERR_NONE) {
            OGR_F_Destroy(feature);
            sqlite3_finalize(statement);
            OSRDestroySpatialReference(spatialReference);
            OGR_DS_Destroy(dataSource);
            return false;
        }

        OGR_F_Destroy(feature);
    }

    const bool writeSucceeded = stepResult == SQLITE_DONE;
    const bool syncSucceeded = OGR_DS_SyncToDisk(dataSource) == OGRERR_NONE;

    sqlite3_finalize(statement);
    OSRDestroySpatialReference(spatialReference);
    OGR_DS_Destroy(dataSource);

    return writeSucceeded && syncSucceeded && std::filesystem::exists(geoJsonPath);
}

bool exportTracksToGeoJsonWithGdal(
    sqlite3* database,
    const std::filesystem::path& geoJsonPath,
    const geosensor::data::SensorOrigin& sensorOrigin
)
{
    std::error_code errorCode;
    std::filesystem::remove(geoJsonPath, errorCode);

    GDALAllRegister();

    OGRSFDriverH driver = OGRGetDriverByName("GeoJSON");
    if (driver == nullptr) {
        return false;
    }

    OGRDataSourceH dataSource =
        OGR_Dr_CreateDataSource(driver, geoJsonPath.string().c_str(), nullptr);
    if (dataSource == nullptr) {
        return false;
    }

    OGRSpatialReferenceH spatialReference = OSRNewSpatialReference(nullptr);
    if (spatialReference == nullptr) {
        OGR_DS_Destroy(dataSource);
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        OSRImportFromEPSG(spatialReference, 4326) != OGRERR_NONE
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    OSRSetAxisMappingStrategy(spatialReference, OAMS_TRADITIONAL_GIS_ORDER);

    OGRLayerH layer = OGR_DS_CreateLayer(
        dataSource,
        "tracks",
        spatialReference,
        wkbLineString,
        nullptr
    );
    if (layer == nullptr) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    if (
        !addGdalField(layer, "target_id", OFTInteger64) ||
        !addGdalField(layer, "point_count", OFTInteger64) ||
        !addGdalField(layer, "first_timestamp_ms", OFTInteger64) ||
        !addGdalField(layer, "last_timestamp_ms", OFTInteger64)
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    if (
        sqlite3_prepare_v2(
            database,
            kExportTracksSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    const OGRFeatureDefnH layerDefinition = OGR_L_GetLayerDefn(layer);
    const int targetIdFieldIndex = OGR_FD_GetFieldIndex(layerDefinition, "target_id");
    const int pointCountFieldIndex = OGR_FD_GetFieldIndex(layerDefinition, "point_count");
    const int firstTimestampFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "first_timestamp_ms");
    const int lastTimestampFieldIndex =
        OGR_FD_GetFieldIndex(layerDefinition, "last_timestamp_ms");

    if (
        targetIdFieldIndex < 0 ||
        pointCountFieldIndex < 0 ||
        firstTimestampFieldIndex < 0 ||
        lastTimestampFieldIndex < 0
    ) {
        sqlite3_finalize(statement);
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    const geosensor::coordinates::CoordinateTransform transform(sensorOrigin);
    std::optional<std::int64_t> currentTargetId;
    std::vector<geosensor::data::TargetPosition> currentPositions;
    std::int64_t firstTimestampMs = 0;
    std::int64_t lastTimestampMs = 0;

    auto emitCurrentTrack = [&]() -> bool {
        if (!currentTargetId.has_value() || currentPositions.size() < 2) {
            currentPositions.clear();
            return true;
        }

        OGRFeatureH feature = OGR_F_Create(layerDefinition);
        if (feature == nullptr) {
            return false;
        }

        OGR_F_SetFieldInteger64(feature, targetIdFieldIndex, *currentTargetId);
        OGR_F_SetFieldInteger64(
            feature,
            pointCountFieldIndex,
            static_cast<GIntBig>(currentPositions.size())
        );
        OGR_F_SetFieldInteger64(
            feature,
            firstTimestampFieldIndex,
            firstTimestampMs
        );
        OGR_F_SetFieldInteger64(
            feature,
            lastTimestampFieldIndex,
            lastTimestampMs
        );

        OGRGeometryH lineGeometry = OGR_G_CreateGeometry(wkbLineString);
        if (lineGeometry == nullptr) {
            OGR_F_Destroy(feature);
            return false;
        }

        for (const auto& point : currentPositions) {
            OGR_G_AddPoint_2D(
                lineGeometry,
                point.geographic.longitudeDeg,
                point.geographic.latitudeDeg
            );
        }

        if (OGR_F_SetGeometryDirectly(feature, lineGeometry) != OGRERR_NONE) {
            OGR_F_Destroy(feature);
            return false;
        }

        if (OGR_L_CreateFeature(layer, feature) != OGRERR_NONE) {
            OGR_F_Destroy(feature);
            return false;
        }

        OGR_F_Destroy(feature);
        currentPositions.clear();
        return true;
    };

    int stepResult = SQLITE_ROW;

    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        const std::int64_t targetId = sqlite3_column_int64(statement, 0);
        const std::int64_t timestampMs = sqlite3_column_int64(statement, 1);
        const auto measurement = geosensor::data::SensorMeasurement {
            .rangeM = sqlite3_column_double(statement, 2),
            .azimuthDeg = sqlite3_column_double(statement, 3),
            .elevationDeg = sqlite3_column_double(statement, 4),
            .intensity = sqlite3_column_double(statement, 5),
        };

        if (!currentTargetId.has_value()) {
            currentTargetId = targetId;
            firstTimestampMs = timestampMs;
        } else if (*currentTargetId != targetId) {
            if (!emitCurrentTrack()) {
                sqlite3_finalize(statement);
                OSRDestroySpatialReference(spatialReference);
                OGR_DS_Destroy(dataSource);
                return false;
            }

            currentTargetId = targetId;
            firstTimestampMs = timestampMs;
        }

        lastTimestampMs = timestampMs;
        currentPositions.push_back(transform.transform(measurement));
    }

    if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    if (!emitCurrentTrack()) {
        sqlite3_finalize(statement);
        OSRDestroySpatialReference(spatialReference);
        OGR_DS_Destroy(dataSource);
        return false;
    }

    const bool syncSucceeded = OGR_DS_SyncToDisk(dataSource) == OGRERR_NONE;

    sqlite3_finalize(statement);
    OSRDestroySpatialReference(spatialReference);
    OGR_DS_Destroy(dataSource);

    return syncSucceeded && std::filesystem::exists(geoJsonPath);
}
#endif

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

std::optional<std::vector<MeasurementDatabase::TrackStatistics>>
MeasurementDatabase::trackStatistics() const
{
    if (database_ == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database_,
            kTrackStatisticsSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return std::nullopt;
    }

    std::vector<TrackStatistics> statistics;
    int stepResult = SQLITE_ROW;

    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        statistics.push_back(TrackStatistics {
            .targetId = sqlite3_column_int64(statement, 0),
            .pointCount = sqlite3_column_int64(statement, 1),
            .firstTimestampMs = sqlite3_column_int64(statement, 2),
            .lastTimestampMs = sqlite3_column_int64(statement, 3),
            .minRangeM = sqlite3_column_double(statement, 4),
            .maxRangeM = sqlite3_column_double(statement, 5),
            .averageIntensity = sqlite3_column_double(statement, 6),
        });
    }

    sqlite3_finalize(statement);

    if (stepResult != SQLITE_DONE) {
        return std::nullopt;
    }

    return statistics;
}

bool MeasurementDatabase::exportMeasurementsToCsv(
    const std::filesystem::path& csvPath
)
{
    if (database_ == nullptr) {
        return false;
    }

    std::ofstream output(csvPath);
    if (!output.is_open()) {
        return false;
    }

    output.imbue(std::locale::classic());

    output
        << "target_id,timestamp_ms,range_m,azimuth_deg,elevation_deg,intensity\n";
    if (!output) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database_,
            kExportMeasurementsSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    int stepResult = SQLITE_ROW;

    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        if (sqlite3_column_type(statement, 0) == SQLITE_NULL) {
            output << ',';
        } else {
            output << sqlite3_column_int64(statement, 0) << ',';
        }

        output
            << sqlite3_column_int64(statement, 1) << ','
            << sqlite3_column_double(statement, 2) << ','
            << sqlite3_column_double(statement, 3) << ','
            << sqlite3_column_double(statement, 4) << ','
            << sqlite3_column_double(statement, 5) << '\n';

        if (!output) {
            sqlite3_finalize(statement);
            return false;
        }
    }

    sqlite3_finalize(statement);
    return stepResult == SQLITE_DONE && static_cast<bool>(output);
}

bool MeasurementDatabase::exportMeasurementsToGeoJson(
    const std::filesystem::path& geoJsonPath,
    const data::SensorOrigin& sensorOrigin
)
{
    if (database_ == nullptr) {
        return false;
    }

#ifdef GEOSENSOR_HAVE_GDAL
    return exportMeasurementsToGeoJsonWithGdal(
        database_,
        geoJsonPath,
        sensorOrigin
    );
#else
    std::ofstream output(geoJsonPath);
    if (!output.is_open()) {
        return false;
    }

    output.imbue(std::locale::classic());

    output << "{\n  \"type\": \"FeatureCollection\",\n  \"features\": [\n";
    if (!output) {
        return false;
    }

    const geosensor::coordinates::CoordinateTransform transform(sensorOrigin);
    sqlite3_stmt* statement = nullptr;

    if (
        sqlite3_prepare_v2(
            database_,
            kExportMeasurementsSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK
    ) {
        return false;
    }

    bool wroteAnyFeature = false;
    int stepResult = SQLITE_ROW;

    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        const auto measurement = data::SensorMeasurement {
            .rangeM = sqlite3_column_double(statement, 2),
            .azimuthDeg = sqlite3_column_double(statement, 3),
            .elevationDeg = sqlite3_column_double(statement, 4),
            .intensity = sqlite3_column_double(statement, 5),
        };
        const auto targetPosition = transform.transform(measurement);

        if (wroteAnyFeature) {
            output << ",\n";
        }
        wroteAnyFeature = true;

        output << "    {\n"
               << "      \"type\": \"Feature\",\n"
               << "      \"geometry\": {\n"
               << "        \"type\": \"Point\",\n"
               << "        \"coordinates\": [";
        writeJsonDouble(output, targetPosition.geographic.longitudeDeg);
        output << ", ";
        writeJsonDouble(output, targetPosition.geographic.latitudeDeg);
        output << "]\n"
               << "      },\n"
               << "      \"properties\": {\n"
               << "        \"target_id\": ";
        if (sqlite3_column_type(statement, 0) == SQLITE_NULL) {
            output << "null";
        } else {
            output << sqlite3_column_int64(statement, 0);
        }
        output << ",\n"
               << "        \"timestamp_ms\": "
               << sqlite3_column_int64(statement, 1) << ",\n"
               << "        \"range_m\": ";
        writeJsonDouble(output, measurement.rangeM);
        output << ",\n"
               << "        \"azimuth_deg\": ";
        writeJsonDouble(output, measurement.azimuthDeg);
        output << ",\n"
               << "        \"elevation_deg\": ";
        writeJsonDouble(output, measurement.elevationDeg);
        output << ",\n"
               << "        \"intensity\": ";
        writeJsonDouble(output, measurement.intensity);
        output << "\n"
               << "      }\n"
               << "    }";

        if (!output) {
            sqlite3_finalize(statement);
            return false;
        }
    }

    output << "\n  ]\n}\n";
    sqlite3_finalize(statement);
    return stepResult == SQLITE_DONE && static_cast<bool>(output);
#endif
}

bool MeasurementDatabase::exportTracksToGeoJson(
    const std::filesystem::path& geoJsonPath,
    const data::SensorOrigin& sensorOrigin
)
{
    if (database_ == nullptr) {
        return false;
    }

#ifdef GEOSENSOR_HAVE_GDAL
    return exportTracksToGeoJsonWithGdal(
        database_,
        geoJsonPath,
        sensorOrigin
    );
#else
    (void)geoJsonPath;
    (void)sensorOrigin;
    return false;
#endif
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
