# GeoSensor Radar Viewer

[![CI](https://github.com/wpop/geosensor-radar-viewer/actions/workflows/ci.yml/badge.svg)](https://github.com/wpop/geosensor-radar-viewer/actions/workflows/ci.yml)

GeoSensor Radar Viewer is a C++20 and Qt6 desktop application for visualizing georeferenced radar-like sensor measurements. It demonstrates a focused workflow for loading CSV data, transforming measurements into local ENU coordinates, and presenting the results in a radar-style view.

## Screenshot

![GeoSensor Radar Viewer main window](docs/images/geosensor-radar-viewer-main.png)

## Current Features

- Qt6 Widgets desktop interface
- CSV loading for radar-style sensor measurements
- Live UDP measurement receiver on `127.0.0.1:5005`
- Python UDP simulator with static and moving target modes
- Live UDP controls for start, stop, and clear
- Range / azimuth / elevation to local ENU coordinate transformation
- Approximate ENU to WGS84 latitude / longitude conversion
- Radar-style 2D visualization with:
  - sensor center
  - range rings
  - range labels
  - separate CSV/sample and live UDP target styles
  - live UDP target buffer limited to the latest 100 positions
  - total valid UDP packet counter
  - legend
- Unit tests for coordinate transformations
- Unit tests for CSV measurement loading

## Example CSV Format

```csv
range_m,azimuth_deg,elevation_deg,intensity
1200.0,45.0,3.0,0.82
950.0,70.0,1.5,0.64
1500.0,120.0,2.0,0.73
600.0,315.0,0.5,0.91
```

## Technology Stack

- C++20
- Qt6 Widgets
- CMake
- Ninja
- CTest

## Build Instructions

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## Run Instructions

```bash
./build/geosensor-radar-viewer
```

## Live UDP Demo

Run the radar viewer in Terminal 1:

```bash
./build/geosensor-radar-viewer
```

Run the static UDP simulator in Terminal 2:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode static --interval 0.5
```

Run the moving UDP simulator in Terminal 2:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode moving --interval 0.2 --azimuth-step 5
```

In the radar view, CSV/sample targets remain visible and are styled separately from live UDP targets. `Start UDP` starts or resumes receiving live packets, `Stop UDP` pauses live receiving, and `Clear Live Targets` removes only the live UDP targets while keeping the CSV/sample targets on screen. Live targets are limited to the latest 100 positions, and the `Total valid UDP packets` status line continues increasing even after the live target buffer reaches 100. In moving mode, azimuth changes gradually so the red live target moves around the radar.

## SQLite Storage

Valid live UDP measurements are stored automatically in `data/geosensor_live_measurements.sqlite` when the application runs. The UI also shows storage status and the current stored measurement count.

The database file is a runtime artifact and should not be committed. The repository ignores `data/*.sqlite`.

`Clear Live Targets` clears only the live display buffer. It does not delete rows already stored in SQLite.

To inspect the current stored row count:

```bash
sqlite3 data/geosensor_live_measurements.sqlite "SELECT COUNT(*) FROM measurements;"
```

## UDP Sensor Simulator

The repository includes a small Python UDP simulator that sends one radar-style measurement per packet as CSV text:

```text
1200.0,45.0,3.0,0.82
```

Run it with the default destination `127.0.0.1:5005` and a `1.0` second interval:

```bash
./scripts/simulator/udp_sensor_simulator.py
```

Run the existing repeating static sample mode explicitly:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode static
```

Run a moving-target mode with gradually changing azimuth:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode moving --azimuth-step 5.0
```

Optional arguments:

- `--host` to change the destination host
- `--port` to change the destination UDP port
- `--interval` to change the delay between packets in seconds
- `--mode` to choose `static` or `moving`
- `--azimuth-step` to control azimuth change per packet in moving mode

## Test Instructions

The project uses simple C++ assert-based test executables registered with CTest.

```bash
ctest --test-dir build --output-on-failure
```

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
scripts/simulator/
```

## Roadmap

- UDP sensor simulator
- Qt UDP receiver
- SQLite storage
- GDAL / PROJ GIS support
- GitHub Actions CI
