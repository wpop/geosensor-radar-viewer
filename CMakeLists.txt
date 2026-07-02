cmake_minimum_required(VERSION 3.22)

project(
    GeoSensorRadarViewer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(SQLite3 REQUIRED)

add_library(geosensor_core STATIC
    include/geosensor/networking/UdpMeasurementReceiver.h
    src/coordinates/CoordinateTransform.cpp
    src/io/CsvMeasurementLoader.cpp
    src/io/UdpMeasurementParser.cpp
    src/networking/UdpMeasurementReceiver.cpp
    src/storage/MeasurementDatabase.cpp
)

target_include_directories(geosensor_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(geosensor_core
    PUBLIC
        Qt6::Network
        SQLite::SQLite3
)

target_compile_options(geosensor_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(geosensor-radar-viewer
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/RadarView.cpp
)

target_include_directories(geosensor-radar-viewer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(geosensor-radar-viewer
    PRIVATE
        Qt6::Widgets
        geosensor_core
)

target_compile_options(geosensor-radar-viewer
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

enable_testing()

add_executable(geosensor_coordinate_tests
    tests/coordinates/coordinate_transform_tests.cpp
)

target_link_libraries(geosensor_coordinate_tests
    PRIVATE
        geosensor_core
)

target_compile_options(geosensor_coordinate_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_test(
    NAME geosensor_coordinate_tests
    COMMAND geosensor_coordinate_tests
)

add_executable(geosensor_csv_tests
    tests/io/csv_measurement_loader_tests.cpp
)

target_link_libraries(geosensor_csv_tests
    PRIVATE
        geosensor_core
)

target_compile_options(geosensor_csv_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_test(
    NAME geosensor_csv_tests
    COMMAND
        geosensor_csv_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/data/samples/measurements.csv
)

add_executable(geosensor_udp_parser_tests
    tests/io/udp_measurement_parser_tests.cpp
)

target_link_libraries(geosensor_udp_parser_tests
    PRIVATE
        geosensor_core
)

target_compile_options(geosensor_udp_parser_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_test(
    NAME geosensor_udp_parser_tests
    COMMAND geosensor_udp_parser_tests
)

add_executable(geosensor_storage_tests
    tests/storage/measurement_database_tests.cpp
)

target_link_libraries(geosensor_storage_tests
    PRIVATE
        geosensor_core
)

target_compile_options(geosensor_storage_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_test(
    NAME geosensor_storage_tests
    COMMAND geosensor_storage_tests
)
