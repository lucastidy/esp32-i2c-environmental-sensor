# ESP32 I2C Environmental Sensor Suite

Reads temperature, humidity, and pressure using:
- AHT20 (temperature + humidity)
- BMP280 (temperature + pressure)

## Features

- ESP-IDF based project
- I2C bus with multiple sensors
- Modular drivers
- Accurate ambient temp from AHT20
- Unified logging to console

## Hardware

### MCU

| Component | Model |
|-----------|--------|
| MCU       | ESP32  |

### Sensors (I²C)

| Sensor  | I²C Address |
|---------|-------------|
| AHT20   | `0x38`      |
| BMP280  | `0x77`      |


Wiring:
- SDA to GPIO26 (or whichever you choose)
- SCL to GPIO25 (again, whichever you choose)
- VCC to 3.3V
- GND to GND

*if other I/O ports are chosen for the SDA/SCL lines, ensure to change:
```bash
#define I2C_MASTER_SDA_IO         26
#define I2C_MASTER_SCL_IO         25
```
inside `i2c_init.h`.

## Build & Flash

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor

Replace `PORT` with the USB port (typically something like `/dev/ttyUSB0`) 
