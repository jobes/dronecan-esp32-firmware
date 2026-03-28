---
description: Build and flash a specific DroneCAN firmware to an ESP32 device.
---

## Build and Flash Workflow

Use this workflow to compile and upload firmware to a specific ESP32 device.

### 1. Identify Target Firmware
Locate the firmware project under `firmwares/` (e.g., `esp32-gateway` or `esp32-airpressure`).

### 2. Enter Firmware Directory
Navigate to the firmware root directory.

### 3. Build & Flash
Run the standard ESP-IDF build and flash command. Replace `/dev/ttyUSB0` with your actual serial port if needed.

// turbo
```bash
idf.py build
```

Then flash to the device:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### 4. Verify Logs
Observe the serial output to ensure DroneCAN initialization and DNA (Dynamic Node ID Allocation) process completed successfully.
