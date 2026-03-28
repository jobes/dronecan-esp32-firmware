---
description: Create a new DroneCAN firmware project based on an existing template.
---

## Add a New Firmware Workflow

This workflow guides you through manually creating a new firmware project for a device.

### 1. Identify Base Template
Choose a firmware in `firmwares/` to act as a baseline (typically `esp32-gateway` for general logic or `esp32-airpressure` for sensor nodes).

### 2. Copy and Initialize
Copy the entire firmware folder to a new name.

```bash
cp -r firmwares/<template> firmwares/esp32-<new-func>
cd firmwares/esp32-<new-func>
```

### 3. Update Build System
In `CMakeLists.txt`, change:
- `project(esp32_<new_func>)`
- Ensure `EXTRA_COMPONENT_DIRS` points correctly to `${REPO_ROOT}/components`.

### 4. Configure Application
In `main/project_config.h`:
- Define a unique `DEVICE_NAME` (e.g., `"com.inskycore.esp32_new"`)
- Set `CAN_TX_PIN` and `CAN_RX_PIN`.

### 5. Build and Test
// turbo
```bash
idf.py build
```
Check that it compiles without errors from the shared components.
