#include "geosensor/coordinates/CoordinateTransform.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace
{

bool nearlyEqual(double lhs, double rhs, double tolerance) noexcept
{
    return std::abs(lhs - rhs) <= tolerance;
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
    assert(nearlyEqual(target.geographic.altitudeM, origin.altitudeM, 1e-9));
}

} // namespace

int main()
{
    testAzimuthNorth();
    testAzimuthEast();
    testElevationUp();
    testFullTransform();

    std::cout << "All coordinate transform tests passed.\n";

    return 0;
}
