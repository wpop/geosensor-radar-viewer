#pragma once

#include "geosensor/data/SensorMeasurement.h"
#include "geosensor/data/SensorOrigin.h"
#include "geosensor/data/TargetPosition.h"

namespace geosensor::coordinates
{

// Converts radar-like polar measurements into local and geographic positions.
//
// This class stores the sensor origin and uses it as the reference point for
// all coordinate transformations.
//
// Current implementation:
// - range / azimuth / elevation -> local ENU
// - local ENU -> approximate WGS84 latitude / longitude
//
// The WGS84 conversion is a local approximation suitable for small distances
// around the sensor. A PROJ-based implementation can be added later for more
// advanced GIS workflows.
class CoordinateTransform
{
public:
    explicit CoordinateTransform(data::SensorOrigin origin) noexcept;

    // Converts a raw sensor measurement into both ENU and WGS84 coordinates.
    [[nodiscard]] data::TargetPosition transform(
        const data::SensorMeasurement& measurement
    ) const noexcept;

    // Converts range / azimuth / elevation into local ENU coordinates.
    //
    // Azimuth convention:
    // - 0 degrees   = North
    // - 90 degrees  = East
    // - 180 degrees = South
    // - 270 degrees = West
    //
    // Elevation convention:
    // - 0 degrees means horizontal direction.
    // - Positive values point upward.
    [[nodiscard]] static data::EnuPosition sensorToEnu(
        const data::SensorMeasurement& measurement
    ) noexcept;

    // Converts local ENU coordinates into approximate WGS84 coordinates.
    [[nodiscard]] static data::GeographicPosition enuToGeographic(
        const data::EnuPosition& enu,
        const data::SensorOrigin& origin
    ) noexcept;

private:
    [[nodiscard]] static double degToRad(double degrees) noexcept;
    [[nodiscard]] static double radToDeg(double radians) noexcept;

    data::SensorOrigin origin_;
};

} // namespace geosensor::coordinates
