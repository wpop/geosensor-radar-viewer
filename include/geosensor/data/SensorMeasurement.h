#pragma once

namespace geosensor::data
{

// Raw polar measurement reported by a radar-like or sonar-like sensor.
//
// Units and conventions:
// - rangeM: distance from the sensor to the target, in metres.
// - azimuthDeg: horizontal angle in degrees.
// - elevationDeg: vertical angle in degrees.
// - intensity: normalized signal strength, usually in the range [0, 1].
struct SensorMeasurement
{
    double rangeM {};
    double azimuthDeg {};
    double elevationDeg {};
    double intensity {};
};

} // namespace geosensor::data
