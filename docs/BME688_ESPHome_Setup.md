# LeafSense BME688 ESPHome Setup

This optional LeafSense device runs a BME688 on a separate ESP32-S3 N16R8 board. It does not modify or depend on the AMG8833 thermal controller.

## Hardware

| BME688 | ESP32-S3 N16R8 |
| --- | --- |
| VIN/3V3 | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

The starting I2C address is `0x76`. The ESPHome log has I2C scanning enabled. If it reports `0x77`, change `bme688_i2c_address` in the device YAML.

## BSEC2 configuration

The configuration uses:

- BME688 regression output.
- LP sampling, approximately one sample every three seconds.
- 28-day background calibration history.
- BSEC2 calibration-state saving every six hours and when full calibration is reached.
- Temperature, humidity, pressure, gas resistance, IAQ, static IAQ, IAQ accuracy, estimated CO2 and breath-VOC entities.

The temperature offset starts at `0.0`. If enclosure heating makes the reading high, set `bme688_temperature_offset` to the amount to subtract.

## Licence boundary

Using the ESPHome BSEC2 component accepts Bosch's BSEC licence. Do not commit or distribute compiled firmware containing BSEC2, Bosch libraries, ESPHome build directories, or downloaded build artifacts. This repository stores source YAML only. The root `.gitignore` blocks common licensed and compiled outputs.

## ESPHome Device Builder

After this branch is pushed, create a new device YAML in `/config/esphome`, then paste the contents of `examples/esphome/bme688_import_from_git.yaml`.

Add this to `/config/esphome/secrets.yaml` if it is not already present:

```yaml
wifi_ssid: "YOUR_WIFI_NAME"
wifi_password: "YOUR_WIFI_PASSWORD"
bme688_api_key: "YOUR_32_BYTE_BASE64_API_KEY"
```

Validate first. Confirm the I2C scan finds `0x76`, then install by USB for the first flash. Later updates can use OTA.

## Calibration expectations

IAQ Accuracy normally starts at `Stabilizing`, then progresses through `Uncertain` or `Calibrating` before reaching `Calibrated`. Keep the device powered and exposed to normal clean and occupied-air conditions. Do not judge IAQ performance from the first few minutes.

