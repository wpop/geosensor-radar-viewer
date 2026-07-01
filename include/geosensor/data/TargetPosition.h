#pragma once

namespace geosensor::data
{

// Local ENU position relative to the sensor origin.
//
// ENU convention:
// - East:  positive X direction, in metres.
// - North: positive Y direction, in metres.
// - Up:    positive Z direction, in metres.
struct EnuPosition
{
    double eastM {};
    double northM {};
    double upM {};
};

// Geographic WGS84 position of a detected target.
struct GeographicPosition
{
    double latitudeDeg {};
    double longitudeDeg {};
    double altitudeM {};
};

// Target position represented in both local and geographic coordinates.
//
// The local ENU position is useful for radar-style visualization.
// The geographic position is useful for map/GIS visualization.
struct TargetPosition
{
    EnuPosition enu;
    GeographicPosition geographic;
};

} // namespace geosensor::data
