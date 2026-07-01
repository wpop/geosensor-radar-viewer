#!/usr/bin/env bash

set -euo pipefail

# This script creates the initial project structure for GeoSensor Radar Viewer.
# Run it from the repository root.

PROJECT_NAME="GeoSensor Radar Viewer"

echo "Creating project structure for ${PROJECT_NAME}..."

mkdir -p include/geosensor/app
mkdir -p include/geosensor/coordinates
mkdir -p include/geosensor/data
mkdir -p include/geosensor/io
mkdir -p include/geosensor/networking
mkdir -p include/geosensor/storage
mkdir -p include/geosensor/ui

mkdir -p src/app
mkdir -p src/coordinates
mkdir -p src/data
mkdir -p src/io
mkdir -p src/networking
mkdir -p src/storage
mkdir -p src/ui

mkdir -p tests/coordinates

mkdir -p scripts/dev
mkdir -p scripts/simulator

mkdir -p data/samples
mkdir -p docs/images
mkdir -p cmake

touch CMakeLists.txt
touch README.md
touch .gitignore

touch src/main.cpp

touch include/geosensor/ui/MainWindow.h
touch src/ui/MainWindow.cpp

touch include/geosensor/coordinates/CoordinateTransform.h
touch src/coordinates/CoordinateTransform.cpp

touch include/geosensor/data/SensorMeasurement.h
touch include/geosensor/data/TargetPosition.h
touch include/geosensor/data/SensorOrigin.h

touch tests/coordinates/coordinate_transform_tests.cpp

touch data/samples/measurements.csv

touch docs/architecture.md

echo "Project structure created successfully."
