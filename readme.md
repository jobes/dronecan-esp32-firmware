# Firmware – installation

## Download

```bash
git clone --recurse-submodules <URL_REPOZITARA>
cd firmware
```

## Submodule update

```bash
git submodule update --remote --recursive
```

. /opt/esp-idf/export.sh
idf.py set-target esp32c3
idf.py -p /dev/ttyACM1 build flash monitor
