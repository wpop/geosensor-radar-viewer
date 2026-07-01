#include "geosensor/coordinates/CoordinateTransform.h"

#include <cmath>
#include <numbers>

namespace
{

// WGS84 ellipsoid constants.
constexpr double kWgs84SemiMajorAxisM = 6378137.0;
constexpr double kWgs84Flattening = 1.0 / 298.257223563;
constexpr double kWgs84EccentricitySquared =
    kWgs84Flattening * (2.0 - kWgs84Flattening);

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
    const double latitudeRad = degToRad(origin.latitudeDeg);
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

    data::GeographicPosition geographic;

    geographic.latitudeDeg =
        origin.latitudeDeg + radToDeg(deltaLatitudeRad);

    geographic.longitudeDeg =
        origin.longitudeDeg + radToDeg(deltaLongitudeRad);

    geographic.altitudeM = origin.altitudeM + enu.upM;

    return geographic;
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
