# esp32-airpressure firmware

## Build and flash

```bash
. /opt/esp-idf/export.sh
cd firmwares/esp32-airpressure
idf.py set-target esp32c3
idf.py -p /dev/ttyACM1 build flash monitor
```

## Notes

- Shared components are loaded from `../../components`.
- Shared submodules are stored at repository root in `../../submodules`.
