# ESP32 Air Pressure Firmware

Firmware for an ESP32-C3 based DroneCAN air data node using a BMP390 pressure sensor over SPI.

## What This Firmware Does

- Initializes the BMP390 sensor over SPI.
- Joins DroneCAN network over TWAI/CAN.
- Uses DroneCAN dynamic node ID allocation at startup.
- Publishes:
  - `uavcan.equipment.air_data.StaticPressure` (type ID `1028`)
  - `uavcan.equipment.air_data.StaticTemperature` (type ID `1029`)
- Publishes heartbeat (`uavcan.protocol.NodeStatus`) approximately every `950 ms`.
- Exposes runtime parameters for pressure and temperature offsets.

## Hardware Configuration

Defined in `main/project_config.h`.

### CAN (TWAI)

- `CAN_TX_PIN`: `GPIO21`
- `CAN_RX_PIN`: `GPIO20`
- Bitrate: `1 Mbit/s` (`TWAI_TIMING_CONFIG_1MBITS`)

Note: ESP32-C3 requires an external CAN transceiver between MCU TWAI pins and CAN bus.

### BMP390 (SPI)

- `BMP_SCLK`: `GPIO4`
- `BMP_MISO`: `GPIO5`
- `BMP_MOSI`: `GPIO6`
- `BMP_CS`: `GPIO7`
- SPI clock: `1 MHz`

## DroneCAN Node Behavior

- Node starts in `MODE_INITIALIZATION`.
- During startup it requests a dynamic node ID using its unique ID.
- After successful initialization, application switches to `MODE_OPERATIONAL`.
- Main loop period is `100 ms`.
- If sensor read or publish fails, node health is degraded (`WARNING/ERROR/CRITICAL` depending on failure).

## Runtime Parameters

The node registers two float parameters (available over DroneCAN param services):

- `pressure_offset`
  - Range: `-100.0 .. 100.0`
  - Default: `0.0`
  - Applied to published pressure in pascals.
- `temp_offset`
  - Range: `-10.0 .. 10.0`
  - Default: `0.0`
  - Applied to published temperature in degrees Celsius.

Parameters are backed by NVS and can be saved/erased through standard DroneCAN parameter opcodes.

## Build and Flash

From repository root:

```bash
. /opt/esp-idf/export.sh
cd firmwares/esp32-airpressure
idf.py set-target esp32c3
idf.py -p /dev/ttyACM1 build flash monitor
```

If target is already set, you can skip `set-target`.

## Useful Commands

```bash
# Build only
idf.py build

# Flash only
idf.py -p /dev/ttyACM1 flash

# Monitor only
idf.py -p /dev/ttyACM1 monitor
```

## Project Structure Notes

- Shared components are taken from `../../components`.
- Shared submodules are located in `../../submodules`.
- Partition table is defined in `partitions.csv` (OTA slots + SPIFFS storage).
