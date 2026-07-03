#include "geosensor/coordinates/CoordinateTransform.h"

#include <cmath>
#include <numbers>
#include <optional>

#ifdef GEOSENSOR_HAVE_PROJ
#include <proj.h>
#endif

namespace
{

// WGS84 ellipsoid constants.
constexpr double kWgs84SemiMajorAxisM = 6378137.0;
constexpr double kWgs84Flattening = 1.0 / 298.257223563;
constexpr double kWgs84EccentricitySquared =
    kWgs84Flattening * (2.0 - kWgs84Flattening);

struct EcefPosition
{
    double xM {};
    double yM {};
    double zM {};
};

EcefPosition geographicToEcef(
    const geosensor::data::GeographicPosition& geographic
) noexcept
{
    const double latitudeRad =
        geographic.latitudeDeg * std::numbers::pi_v<double> / 180.0;
    const double longitudeRad =
        geographic.longitudeDeg * std::numbers::pi_v<double> / 180.0;

    const double sinLatitude = std::sin(latitudeRad);
    const double cosLatitude = std::cos(latitudeRad);
    const double sinLongitude = std::sin(longitudeRad);
    const double cosLongitude = std::cos(longitudeRad);

    const double primeVerticalRadiusM =
        kWgs84SemiMajorAxisM /
        std::sqrt(1.0 - kWgs84EccentricitySquared * sinLatitude * sinLatitude);

    EcefPosition ecef;

    ecef.xM =
        (primeVerticalRadiusM + geographic.altitudeM) *
        cosLatitude *
        cosLongitude;
    ecef.yM =
        (primeVerticalRadiusM + geographic.altitudeM) *
        cosLatitude *
        sinLongitude;
    ecef.zM =
        (
            primeVerticalRadiusM * (1.0 - kWgs84EccentricitySquared) +
            geographic.altitudeM
        ) * sinLatitude;

    return ecef;
}

EcefPosition enuToEcef(
    const geosensor::data::EnuPosition& enu,
    const geosensor::data::GeographicPosition& originGeographic
) noexcept
{
    const double latitudeRad =
        originGeographic.latitudeDeg * std::numbers::pi_v<double> / 180.0;
    const double longitudeRad =
        originGeographic.longitudeDeg * std::numbers::pi_v<double> / 180.0;

    const double sinLatitude = std::sin(latitudeRad);
    const double cosLatitude = std::cos(latitudeRad);
    const double sinLongitude = std::sin(longitudeRad);
    const double cosLongitude = std::cos(longitudeRad);

    EcefPosition ecef;

    ecef.xM =
        (-sinLongitude * enu.eastM) +
        (-sinLatitude * cosLongitude * enu.northM) +
        (cosLatitude * cosLongitude * enu.upM);
    ecef.yM =
        (cosLongitude * enu.eastM) +
        (-sinLatitude * sinLongitude * enu.northM) +
        (cosLatitude * sinLongitude * enu.upM);
    ecef.zM =
        (cosLatitude * enu.northM) + (sinLatitude * enu.upM);

    return ecef;
}

geosensor::data::GeographicPosition enuToGeographicApproximate(
    const geosensor::data::EnuPosition& enu,
    const geosensor::data::SensorOrigin& origin
) noexcept
{
    const double latitudeRad =
        origin.latitudeDeg * std::numbers::pi_v<double> / 180.0;
    const double sinLatitude = std::sin(latitudeRad);
    const double cosLatitude = std::cos(latitudeRad);

    const double wgs84RadiusDenominator =
        std::sqrt(
            1.0 -
            kWgs84EccentricitySquared * sinLatitude * sinLatitude
        );

    const double primeVerticalRadiusM =
        kWgs84SemiMajorAxisM / wgs84RadiusDenominator;

    const double meridianRadiusM =
        kWgs84SemiMajorAxisM *
        (1.0 - kWgs84EccentricitySquared) /
        std::pow(
            1.0 -
            kWgs84EccentricitySquared * sinLatitude * sinLatitude,
            1.5
        );

    const double deltaLatitudeRad = enu.northM / meridianRadiusM;
    const double deltaLongitudeRad =
        enu.eastM / (primeVerticalRadiusM * cosLatitude);

    geosensor::data::GeographicPosition geographic;

    geographic.latitudeDeg =
        origin.latitudeDeg + deltaLatitudeRad * 180.0 / std::numbers::pi_v<double>;

    geographic.longitudeDeg =
        origin.longitudeDeg +
        deltaLongitudeRad * 180.0 / std::numbers::pi_v<double>;

    geographic.altitudeM = origin.altitudeM + enu.upM;

    return geographic;
}

#ifdef GEOSENSOR_HAVE_PROJ
std::optional<geosensor::data::GeographicPosition> enuToGeographicWithProj(
    const geosensor::data::EnuPosition& enu,
    const geosensor::data::SensorOrigin& origin
)
{
    const EcefPosition originEcef =
        geographicToEcef(geosensor::data::GeographicPosition {
            .latitudeDeg = origin.latitudeDeg,
            .longitudeDeg = origin.longitudeDeg,
            .altitudeM = origin.altitudeM,
        });

    const EcefPosition offset = enuToEcef(
        enu,
        geosensor::data::GeographicPosition {
            .latitudeDeg = origin.latitudeDeg,
            .longitudeDeg = origin.longitudeDeg,
            .altitudeM = origin.altitudeM,
        }
    );

    const EcefPosition ecef {
        .xM = originEcef.xM + offset.xM,
        .yM = originEcef.yM + offset.yM,
        .zM = originEcef.zM + offset.zM,
    };

    PJ_CONTEXT* context = proj_context_create();
    if (context == nullptr) {
        return std::nullopt;
    }

    PJ* rawTransform =
        proj_create_crs_to_crs(context, "EPSG:4978", "EPSG:4979", nullptr);
    if (rawTransform == nullptr) {
        proj_context_destroy(context);
        return std::nullopt;
    }

    PJ* transform = proj_normalize_for_visualization(context, rawTransform);
    proj_destroy(rawTransform);

    if (transform == nullptr) {
        proj_context_destroy(context);
        return std::nullopt;
    }

    const PJ_COORD result = proj_trans(
        transform,
        PJ_FWD,
        proj_coord(ecef.xM, ecef.yM, ecef.zM, 0.0)
    );

    proj_destroy(transform);
    proj_context_destroy(context);

    if (
        !std::isfinite(result.lpz.lam) ||
        !std::isfinite(result.lpz.phi) ||
        !std::isfinite(result.lpz.z)
    ) {
        return std::nullopt;
    }

    geosensor::data::GeographicPosition geographic;
    geographic.longitudeDeg = result.lpz.lam;
    geographic.latitudeDeg = result.lpz.phi;
    geographic.altitudeM = result.lpz.z;

    return geographic;
}
#endif

} // namespace

namespace geosensor::coordinates
{

CoordinateTransform::CoordinateTransform(data::SensorOrigin origin) noexcept
    : origin_(origin)
{
}

data::TargetPosition CoordinateTransform::transform(
    const data::SensorMeasurement& measurement
) const noexcept
{
    data::TargetPosition target;

    target.enu = sensorToEnu(measurement);
    target.geographic = enuToGeographic(target.enu, origin_);

    return target;
}

data::EnuPosition CoordinateTransform::sensorToEnu(
    const data::SensorMeasurement& measurement
) noexcept
{
    const double azimuthRad = degToRad(measurement.azimuthDeg);
    const double elevationRad = degToRad(measurement.elevationDeg);

    const double horizontalRangeM =
        measurement.rangeM * std::cos(elevationRad);

    data::EnuPosition enu;

    enu.eastM = horizontalRangeM * std::sin(azimuthRad);
    enu.northM = horizontalRangeM * std::cos(azimuthRad);
    enu.upM = measurement.rangeM * std::sin(elevationRad);

    return enu;
}

data::GeographicPosition CoordinateTransform::enuToGeographic(
    const data::EnuPosition& enu,
    const data::SensorOrigin& origin
) noexcept
{
    #ifdef GEOSENSOR_HAVE_PROJ
    if (const auto projectedGeographic = enuToGeographicWithProj(enu, origin)) {
        return *projectedGeographic;
    }
    #endif

    return enuToGeographicApproximate(enu, origin);
}

double CoordinateTransform::degToRad(double degrees) noexcept
{
    return degrees * std::numbers::pi_v<double> / 180.0;
}

double CoordinateTransform::radToDeg(double radians) noexcept
{
    return radians * 180.0 / std::numbers::pi_v<double>;
}

} // namespace geosensor::coordinates
