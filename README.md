# GeoSensor Radar Viewer

[![CI](https://github.com/wpop/geosensor-radar-viewer/actions/workflows/ci.yml/badge.svg)](https://github.com/wpop/geosensor-radar-viewer/actions/workflows/ci.yml)

GeoSensor Radar Viewer is a C++20 and Qt6 desktop application for visualizing georeferenced radar-like sensor measurements. It demonstrates a focused workflow for loading CSV data, transforming measurements into local ENU coordinates, and presenting the results in a radar-style view.

## Screenshot

![GeoSensor Radar Viewer main window](docs/images/geosensor-radar-viewer-main.png)

## Current Features

- Qt6 Widgets desktop interface
- CSV loading for radar-style sensor measurements
- Live UDP measurement receiver on `127.0.0.1:5005`
- Python UDP simulator with static, moving, and multi-target modes
- UDP payloads can include `target_id` for track-aware visualization
- Live UDP controls for start, stop, and clear
- Range / azimuth / elevation to local ENU coordinate transformation
- Approximate ENU to WGS84 latitude / longitude conversion
- Radar-style 2D visualization with:
  - sensor center
  - range rings
  - range labels
  - separate CSV/sample and live UDP target styles
  - short per-target trails for UDP packets that include `target_id`
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

Run the multi-target UDP simulator in Terminal 2:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode multi --interval 0.2 --azimuth-step 5
```

In the radar view, CSV/sample targets remain visible and are styled separately from live UDP targets. `Start UDP` starts or resumes receiving live packets, `Stop UDP` pauses live receiving, and `Clear Live Targets` removes only the live UDP targets while keeping the CSV/sample targets on screen. Live targets are limited to the latest 100 positions, and the `Total valid UDP packets` status line continues increasing even after the live target buffer reaches 100. In moving mode, azimuth changes gradually so the red live target moves around the radar. When UDP packets include `target_id`, the viewer groups detections into short per-target trails. Legacy 4-field UDP packets still appear as live detections, but they do not carry track identity.

## SQLite Storage

Valid live UDP measurements are stored automatically in `data/geosensor_live_measurements.sqlite` when the application runs. The UI also shows storage status and the current stored measurement count.

The `measurements` table stores:

- `target_id` as a nullable track identifier
- `timestamp_ms` in Unix milliseconds
- `range_m`
- `azimuth_deg`
- `elevation_deg`
- `intensity`

Packets with `target_id` are stored with that value. Legacy 4-field packets are stored with `NULL` `target_id`, which also applies to older rows created before track-aware storage was added. Existing SQLite databases are migrated in place by adding any missing columns.

The database file is a runtime artifact and should not be committed. The repository ignores `data/*.sqlite`.

`Clear Live Targets` clears only the live display buffer. It does not delete rows already stored in SQLite.

To inspect the current stored row count:

```bash
sqlite3 data/geosensor_live_measurements.sqlite "SELECT COUNT(*) FROM measurements;"
```

To see how many stored rows belong to each target identity, including legacy `NULL` rows:

```bash
sqlite3 data/geosensor_live_measurements.sqlite "SELECT COALESCE(CAST(target_id AS TEXT), 'NULL') AS target_id, COUNT(*) AS row_count FROM measurements GROUP BY target_id ORDER BY target_id;"
```

## UDP Sensor Simulator

The repository includes a small Python UDP simulator that sends one radar-style measurement per packet as CSV text.

Supported UDP payload formats:

```text
1200.0,45.0,3.0,0.82
7,1200.0,45.0,3.0,0.82
```

The 4-field format is kept for backward compatibility. The 5-field format adds `target_id` as the first field and enables target-aware trail visualization in the viewer.

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

Run a multi-target mode with several moving detections:

```bash
./scripts/simulator/udp_sensor_simulator.py --mode multi --azimuth-step 5.0
```

In multi-target mode, the simulator sends one UDP packet per target for each update cycle. These packets include `target_id`, so the viewer can render short per-target trails. Legacy 4-field packets still work and show up as live detections without track identity.

Optional arguments:

- `--host` to change the destination host
- `--port` to change the destination UDP port
- `--interval` to change the delay between packets in seconds
- `--mode` to choose `static`, `moving`, or `multi`
- `--azimuth-step` to control azimuth change per packet in moving and multi modes

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
