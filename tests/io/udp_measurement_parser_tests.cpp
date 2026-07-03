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

    const auto packet = geosensor::io::UdpMeasurementParser::parsePacket(
        "7,1200.0,45.0,3.0,0.82"
    );

    assert(packet.has_value());
    assert(packet->targetId.has_value());
    assert(packet->targetId.value() == 7);
    assert(nearlyEqual(packet->measurement.rangeM, 1200.0, 1e-9));
    assert(nearlyEqual(packet->measurement.azimuthDeg, 45.0, 1e-9));
    assert(nearlyEqual(packet->measurement.elevationDeg, 3.0, 1e-9));
    assert(nearlyEqual(packet->measurement.intensity, 0.82, 1e-9));

    const auto packetMeasurement = geosensor::io::UdpMeasurementParser::parse(
        "7,1200.0,45.0,3.0,0.82"
    );

    assert(packetMeasurement.has_value());
    assert(nearlyEqual(packetMeasurement->rangeM, 1200.0, 1e-9));
    assert(nearlyEqual(packetMeasurement->azimuthDeg, 45.0, 1e-9));
    assert(nearlyEqual(packetMeasurement->elevationDeg, 3.0, 1e-9));
    assert(nearlyEqual(packetMeasurement->intensity, 0.82, 1e-9));

    const auto legacyPacket = geosensor::io::UdpMeasurementParser::parsePacket(
        "7,1200.0,45.0,3.0"
    );

    assert(legacyPacket.has_value());
    assert(!legacyPacket->targetId.has_value());
    assert(nearlyEqual(legacyPacket->measurement.rangeM, 7.0, 1e-9));
    assert(nearlyEqual(legacyPacket->measurement.azimuthDeg, 1200.0, 1e-9));
    assert(nearlyEqual(legacyPacket->measurement.elevationDeg, 45.0, 1e-9));
    assert(nearlyEqual(legacyPacket->measurement.intensity, 3.0, 1e-9));

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
    assert(
        !geosensor::io::UdpMeasurementParser::parsePacket(
            "target-a,1200.0,45.0,3.0,0.82"
        )
             .has_value()
    );
    assert(
        !geosensor::io::UdpMeasurementParser::parsePacket(
            "7,1200.0,45.0,3.0,0.82,extra"
        )
             .has_value()
    );

    std::cout << "All UDP measurement parser tests passed.\n";

    return 0;
}
