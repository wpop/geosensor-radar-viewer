# GeoSensor Radar Viewer

GeoSensor Radar Viewer is a C++20 / Qt6 desktop application for visualizing georeferenced radar-like sensor measurements.

The project demonstrates a sensor-data workflow relevant to C++ software development for radar, sonar, mapping, GIS, and visualization systems.

## Project Structure

```text
include/geosensor/
├── coordinates/
├── data/
├── io/
└── ui/

src/
├── coordinates/
├── io/
└── ui/

tests/
├── coordinates/
└── io/

data/samples/
docs/images/

## Screenshot

![GeoSensor Radar Viewer main window](docs/images/geosensor-radar-viewer-main.png)

## Current Features

- Qt6 Widgets desktop GUI
- CSV loading for radar-like measurements
- Range / azimuth / elevation to local ENU coordinate transformation
- Approximate ENU to WGS84 latitude / longitude conversion
- Radar-style 2D visualization with:
  - sensor center
  - range rings
  - range labels
  - detected targets
  - legend
- Unit tests for coordinate transformations
- Unit tests for CSV measurement loading
- CMake / Ninja build workflow

## Example CSV Format

```csv
range_m,azimuth_deg,elevation_deg,intensity
1200.0,45.0,3.0,0.82
950.0,70.0,1.5,0.64
1500.0,120.0,2.0,0.73
600.0,315.0,0.5,0.91
```
