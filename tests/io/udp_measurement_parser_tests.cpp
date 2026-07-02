#include "geosensor/io/UdpMeasurementParser.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace
{

bool nearlyEqual(double lhs, double rhs, double tolerance) noexcept
{
    return std::abs(lhs - rhs) <= tolerance;
}

} // namespace

int main()
{
    const auto measurement =
        geosensor::io::UdpMeasurementParser::parse("1200.0,45.0,3.0,0.82");

    assert(measurement.has_value());
    assert(nearlyEqual(measurement->rangeM, 1200.0, 1e-9));
    assert(nearlyEqual(measurement->azimuthDeg, 45.0, 1e-9));
    assert(nearlyEqual(measurement->elevationDeg, 3.0, 1e-9));
    assert(nearlyEqual(measurement->intensity, 0.82, 1e-9));

    const auto spacedMeasurement =
        geosensor::io::UdpMeasurementParser::parse(" 950.0, 70.0, 1.5, 0.64 ");

    assert(spacedMeasurement.has_value());
    assert(nearlyEqual(spacedMeasurement->rangeM, 950.0, 1e-9));
    assert(nearlyEqual(spacedMeasurement->azimuthDeg, 70.0, 1e-9));
    assert(nearlyEqual(spacedMeasurement->elevationDeg, 1.5, 1e-9));
    assert(nearlyEqual(spacedMeasurement->intensity, 0.64, 1e-9));

    assert(!geosensor::io::UdpMeasurementParser::parse("").has_value());
    assert(!geosensor::io::UdpMeasurementParser::parse("1200.0,45.0,3.0").has_value());
    assert(
        !geosensor::io::UdpMeasurementParser::parse(
            "1200.0,45.0,3.0,0.82,extra"
        )
             .has_value()
    );
    assert(
        !geosensor::io::UdpMeasurementParser::parse(
            "1200.0,not-a-number,3.0,0.82"
        )
             .has_value()
    );

    std::cout << "All UDP measurement parser tests passed.\n";

    return 0;
}
