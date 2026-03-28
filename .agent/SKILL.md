---
name: DroneCAN Firmware Developer
description: Expert in DroneCAN firmware development for ESP32 using ESP-IDF.
---

# DroneCAN ESP32 Firmware Developer Skill

This skill provides expert guidance and automated workflows for developing DroneCAN-based firmware on ESP32 microcontrollers using the ESP-IDF framework.

## Project Structure

- `components/`: Shared ESP-IDF components.
  - `dronecan_mini/`: High-level DroneCAN abstraction (DNA, Parameters, Firmware Update).
  - `libcanard/`: ESP-IDF wrapper for the libcanard library.
- `firmwares/`: Individual firmware projects for different devices.
  - `esp32-airpressure/`: Air pressure sensor node.
  - `esp32-gateway/`: General purpose DroneCAN gateway.
- `submodules/`: External libraries as Git submodules.

## Core Technologies

- **ESP-IDF**: The official development framework for ESP32.
- **DroneCAN**: A lightweight, robust CAN bus protocol for UAVs and robotics.
- **Libcanard**: The underlying DroneCAN implementation in C.

## Multi-Firmware Scalability Strategy

This project is designed to scale to dozens of unique firmware targets. Follow these guidelines to keep the project clean:

### 1. Unified Shared Components
- All logic that can be reused (DroneCAN init, DNA, Heartbeat, Common Messages) must live in `components/dronecan_mini/`.
- Sensor-specific drivers (e.g., BMP380, INA226) should be added as separate components in `components/`

### 2. Standardized Firmware Project Layout
Each firmware in `firmwares/` should follow this template:
- `main/main.c`: The entry point and firmware-specific logic.
- `main/project_config.h`: Key hardware settings (pins, node names, timings).
- `partitions.csv`: Custom partition layout.
- `CMakeLists.txt`: Renames the project and links shared components.
- `readme.md`: Documents HW pinout, specific DroneCAN IDs used, and build notes.

### 3. Naming Conventions
- Firmware folders: `esp32-<functionality>` (e.g., `esp32-egt-sensor`, `esp32-fuel-flow`).
- Node names: `com.inskycore.<firmware-name>` (defined in `project_config.h`).

## Development Workflow

### 1. Build & Flash
Always build and flash from the specific firmware directory under `firmwares/`.

```bash
cd firmwares/<firmware-name>
idf.py build flash monitor
```

### 2. DroneCAN Message Handling
- Shared message definitions are in `components/dronecan_mini/messages/`.
- Receiver logic is in `dronecan_receiver.c`.
- Transmitter logic is typically in `main.c` or a dedicated task.

### 3. Parameters & Configuration
- Per-project configuration is in `main/project_config.h`.
- Custom parameters should be registered in the `dronecan_mini` parameter system.

## Best Practices

1. **Reusability**: Keep protocol-specific and sensor-abstracted logic in `components/`.
2. **Firmware Decoupling**: Keep board-specific pinouts and hardware initialization in the `firmwares/` project's `main/` directory.
3. **Serial Monitoring**: Use `idf.py monitor` for real-time logs.
4. **Git Submodules**: Ensure submodules are updated using `git submodule update --init --recursive`.

## Useful Commands

- `idf.py -p <PORT> build flash monitor`: Complete cycle for a specific firmware.
- `idf.py menuconfig`: Configure firmware settings (WiFi, CAN pins, etc.).
- `idf.py fullclean`: If you encounter weird build issues.
