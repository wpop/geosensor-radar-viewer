#pragma once

namespace geosensor::data
{

// Geographic position of the sensor platform.
//
// Coordinate system:
// - WGS84 latitude / longitude in degrees.
// - Altitude in metres above the WGS84 ellipsoid.
struct SensorOrigin
{
    double latitudeDeg {};
    double longitudeDeg {};
    double altitudeM {};
};

} // namespace geosensor::data
