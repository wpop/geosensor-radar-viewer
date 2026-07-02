#include "geosensor/io/CsvMeasurementLoader.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace
{

bool nearlyEqual(double lhs, double rhs, double tolerance) noexcept
{
    return std::abs(lhs - rhs) <= tolerance;
}

} // namespace

int main(int argc, char* argv[])
{
    assert(argc == 2);

    const std::filesystem::path csvPath = argv[1];

    const geosensor::io::CsvMeasurementLoader loader;
    const auto measurements = loader.load(csvPath);

    assert(measurements.size() == 4);

    assert(nearlyEqual(measurements[0].rangeM, 1200.0, 1e-9));
    assert(nearlyEqual(measurements[0].azimuthDeg, 45.0, 1e-9));
    assert(nearlyEqual(measurements[0].elevationDeg, 3.0, 1e-9));
    assert(nearlyEqual(measurements[0].intensity, 0.82, 1e-9));

    assert(nearlyEqual(measurements[1].rangeM, 950.0, 1e-9));
    assert(nearlyEqual(measurements[1].azimuthDeg, 70.0, 1e-9));

    std::cout << "All CSV measurement loader tests passed.\n";

    return 0;
}
