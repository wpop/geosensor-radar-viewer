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

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(geosensor_core
    src/coordinates/CoordinateTransform.cpp
)

target_include_directories(geosensor_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
