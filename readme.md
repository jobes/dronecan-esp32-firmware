# InSkyCore DroneCAN Firmware Platform

Low-cost ESP32-based avionics devices for experimental aviation.

This repository is a multi-firmware platform for building DroneCAN nodes and sensors for:

- experimental and ultralight airplanes
- powered hang gliders
- custom cockpit projects

The practical goal is simple: publish important flight and engine data over DroneCAN so the pilot can see everything critical on a tablet in the cockpit.

## Vision

This project focuses on:

- affordable hardware built around ESP32
- reusable DroneCAN software components
- multiple device firmwares in one repository
- fast iteration for real-world flying needs

## Safety Notice

This project is intended for experimental use.

It is not a certified avionics system and must not be treated as the sole source of flight-critical information. Always keep independent, appropriate backup instrumentation.

## Repository Layout

```text
firmware/
	components/                  # Shared ESP-IDF components used by all firmwares
		dronecan_mini/             # DroneCAN communication, node state, params, FW update
		bmp3_sensor_api/           # BMP3/BMP390 sensor integration component
		libcanard/                 # ESP-IDF wrapper/component for libcanard

	firmwares/
		<firmware-name>/           # Individual firmware projects (each has its own README)

	submodules/
		libcanard/                 # Upstream libcanard submodule
		bmp3_sensor_api/           # Upstream BMP3 sensor API submodule
```

## Firmware-Specific Documentation

Each firmware under `firmwares/` has (or should have) its own `readme.md` with:

- hardware details and pinout
- published DroneCAN messages/services
- target MCU and board notes
- exact build/flash commands
- calibration and test notes

## Requirements

- Linux development environment
- ESP-IDF installed (compatible with your project setup)
- USB access to target board
- CAN transceiver and proper wiring for DroneCAN bus

## Quick Start

Clone with submodules:

```bash
git clone --recurse-submodules <REPO_URL>
cd firmware
```

If already cloned, update submodules:

```bash
git submodule update --init --recursive
```

Load ESP-IDF environment:

```bash
. /opt/esp-idf/export.sh
```

Build and flash a selected firmware:

```bash
cd firmwares/<firmware-name>
# Follow target selection and configuration in that firmware's readme.md
idf.py -p /dev/ttyACM1 build flash monitor
```

## How To Add a New Device Firmware

1. Create a new folder under `firmwares/`.
2. Add a standard ESP-IDF project (`CMakeLists.txt`, `main/`, config files).
3. Reuse shared logic from `../../components` (especially `dronecan_mini`).
4. Define per-device settings in `main/project_config.h`.
5. Add sensor/application code in `main/` and publish required DroneCAN topics.
6. Build, flash, and validate on a real CAN bus.

## Design Principles

- Keep common DroneCAN logic in shared components.
- Keep firmware-specific hardware details in each firmware folder.
- Prefer explicit parameters for in-field calibration.
- Optimize for robustness and predictable behavior over features.

## Roadmap Ideas

- additional air-data and environmental sensors
- engine monitoring nodes (EGT/CHT, RPM, pressures)
- electrical system telemetry (voltage/current/battery)
- GPS/attitude helper nodes for glass-cockpit integration
- standardized tablet-friendly data profiles

## Contribution

Contributions and practical flight-test feedback are welcome.

Recommended contribution style:

- keep commits focused and small
- document pinout/protocol assumptions
- include test notes (bench or flight context)

## License

Add the project license information here (for example MIT, BSD, or GPL) once finalized.
