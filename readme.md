# Multi-firmware repository

This repository now holds shared dependencies for multiple ESP-IDF firmwares.

## Structure

```text
components/                    # shared ESP-IDF components
submodules/                    # shared git submodules
firmwares/
	esp32-airpressure/           # first firmware project
```

## Clone

```bash
git clone --recurse-submodules <REPO_URL>
cd firmware
```

## Update submodules

```bash
git submodule update --remote --recursive
```

## Build esp32-airpressure

```bash
. /opt/esp-idf/export.sh
cd firmwares/esp32-airpressure
idf.py set-target esp32c3
idf.py -p /dev/ttyACM1 build flash monitor
```

To add another firmware, create a new folder under `firmwares/` with its own ESP-IDF project files (`CMakeLists.txt`, `main/`, `sdkconfig`, `partitions.csv`).
