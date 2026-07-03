#include "geosensor/coordinates/CoordinateTransform.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>

namespace
{

bool nearlyEqual(double lhs, double rhs, double tolerance) noexcept
{
    return std::abs(lhs - rhs) <= tolerance;
}

geosensor::data::GeographicPosition approximateEnuToGeographic(
    const geosensor::data::EnuPosition& enu,
    const geosensor::data::SensorOrigin& origin
)
{
    constexpr double kWgs84SemiMajorAxisM = 6378137.0;
    constexpr double kWgs84Flattening = 1.0 / 298.257223563;
    constexpr double kWgs84EccentricitySquared =
        kWgs84Flattening * (2.0 - kWgs84Flattening);

    const double latitudeRad = origin.latitudeDeg * std::numbers::pi_v<double> / 180.0;
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

void testAzimuthNorth()
{
    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 100.0,
        .azimuthDeg = 0.0,
        .elevationDeg = 0.0,
        .intensity = 1.0
    };

    const auto enu =
        geosensor::coordinates::CoordinateTransform::sensorToEnu(measurement);

    assert(nearlyEqual(enu.eastM, 0.0, 1e-9));
    assert(nearlyEqual(enu.northM, 100.0, 1e-9));
    assert(nearlyEqual(enu.upM, 0.0, 1e-9));
}

void testAzimuthEast()
{
    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 100.0,
        .azimuthDeg = 90.0,
        .elevationDeg = 0.0,
        .intensity = 1.0
    };

    const auto enu =
        geosensor::coordinates::CoordinateTransform::sensorToEnu(measurement);

    assert(nearlyEqual(enu.eastM, 100.0, 1e-9));
    assert(nearlyEqual(enu.northM, 0.0, 1e-9));
    assert(nearlyEqual(enu.upM, 0.0, 1e-9));
}

void testElevationUp()
{
    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 100.0,
        .azimuthDeg = 0.0,
        .elevationDeg = 90.0,
        .intensity = 1.0
    };

    const auto enu =
        geosensor::coordinates::CoordinateTransform::sensorToEnu(measurement);

    assert(nearlyEqual(enu.eastM, 0.0, 1e-9));
    assert(nearlyEqual(enu.northM, 0.0, 1e-9));
    assert(nearlyEqual(enu.upM, 100.0, 1e-9));
}

void testFullTransform()
{
    const geosensor::data::SensorOrigin origin {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };

    const geosensor::data::SensorMeasurement measurement {
        .rangeM = 100.0,
        .azimuthDeg = 0.0,
        .elevationDeg = 0.0,
        .intensity = 1.0
    };

    const geosensor::coordinates::CoordinateTransform transform(origin);
    const auto target = transform.transform(measurement);

    assert(nearlyEqual(target.enu.eastM, 0.0, 1e-9));
    assert(nearlyEqual(target.enu.northM, 100.0, 1e-9));
    assert(nearlyEqual(target.enu.upM, 0.0, 1e-9));

    assert(target.geographic.latitudeDeg > origin.latitudeDeg);
    assert(nearlyEqual(
        target.geographic.longitudeDeg,
        origin.longitudeDeg,
        1e-6
    ));
    assert(nearlyEqual(target.geographic.altitudeM, origin.altitudeM, 1e-3));
}

#ifdef GEOSENSOR_HAVE_PROJ
void testProjAndApproximateGeographicTransformAgree()
{
    const geosensor::data::SensorOrigin origin {
        .latitudeDeg = 49.2488,
        .longitudeDeg = -122.9805,
        .altitudeM = 50.0
    };

    const geosensor::data::EnuPosition enu {
        .eastM = 1500.0,
        .northM = -750.0,
        .upM = 120.0
    };

    const auto projected =
        geosensor::coordinates::CoordinateTransform::enuToGeographic(
            enu,
            origin
        );
    const auto approximate = approximateEnuToGeographic(enu, origin);

    assert(nearlyEqual(projected.latitudeDeg, approximate.latitudeDeg, 1e-5));
    assert(nearlyEqual(projected.longitudeDeg, approximate.longitudeDeg, 1e-5));
    assert(std::isfinite(projected.altitudeM));
}
#endif

} // namespace

int main()
{
    testAzimuthNorth();
    testAzimuthEast();
    testElevationUp();
    testFullTransform();
#ifdef GEOSENSOR_HAVE_PROJ
    testProjAndApproximateGeographicTransformAgree();
#endif

    std::cout << "All coordinate transform tests passed.\n";

    return 0;
}
