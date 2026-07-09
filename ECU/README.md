
## Source
```sh
source /home/ehab/zephyrproject/zephyr/.venv/bin/activate
```

## Build
``` bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu . -DPython3_EXECUTABLE=/home/ehab/zephyrproject/.venv/bin/python3
```

## Flash
``` bash
west flash --esp-device /dev/ttyACM0
```

## Monitor
``` sh
pip install esp-idf-monitor
python -m esp_idf_monitor --port /dev/ttyACM0 --baud 115200 build/zephyr/zephyr.elf
# to terminate
# ctrl + ]
```
