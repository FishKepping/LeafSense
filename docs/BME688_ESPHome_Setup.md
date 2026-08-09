# LeafSense BME688 ESPHome Setup

This optional LeafSense device runs a BME688 on a separate ESP32-S3 N16R8 board. It does not modify or depend on the AMG8833 thermal controller.

## Hardware

| BME688 | ESP32-S3 N16R8 |
| --- | --- |
| VIN/3V3 | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |
| SDO | GND for address `0x76` |
| CS | 3V3 to enable I2C mode |

The starting I2C address is `0x76`. The ESPHome log has I2C scanning enabled. If it reports `0x77`, connect SDO to 3V3 and change `bme688_i2c_address` in the device YAML. Do not leave CS or SDO floating: CS must be high for I2C operation and SDO selects the address.

## BSEC2 configuration

The configuration uses ESPHome's standard BME688 classification profile:

- BME688 classification output. This is the compatible 3.3 V, LP profile for the breakout board; the BSEC2 regression profile requires the specific 1.8 V, ULP, 4-day combination enforced by ESPHome and is not appropriate for this wiring.
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

## Air and leaf VPD

The BME688 device publishes `LeafSense Air VPD` in kPa from its compensated air temperature and relative humidity. It uses the Tetens saturation vapour pressure equation:

```text
air VPD = SVP(air temperature) × (1 - relative humidity / 100)
```

LeafSense also includes `homeassistant/leafsense-vpd-template-sensors.yaml`. This creates six Home Assistant leaf-VPD entities, one for each thermal ROI channel. The calculation uses the ROI average as leaf temperature:

```text
leaf temperature offset = ROI average temperature - air temperature
leaf VPD = SVP(ROI average temperature) - (RH / 100 × SVP(air temperature))
```

This matters because relative humidity describes the air vapour pressure at the **air** temperature. Applying the same RH percentage directly at leaf temperature would give the wrong leaf-to-air vapour pressure difference.

Copy the template file to `/config/leafsense-vpd-template-sensors.yaml`, then add this to Home Assistant's `configuration.yaml`:

```yaml
template: !include leafsense-vpd-template-sensors.yaml
```

If `configuration.yaml` already has a `template:` section, copy the `- sensor:` list item from the LeafSense file into the existing section instead of adding a second `template:` key. Check the source entity IDs in **Developer Tools → States** and replace the defaults in the template file if Home Assistant generated different IDs. Run **Developer Tools → YAML → Check configuration**, then restart Home Assistant or reload Template Entities.

An unused or disabled ROI has no numeric average temperature, so its leaf-VPD entity is unavailable. Each active leaf-VPD entity also exposes `leaf_temperature_offset_c` as an attribute.

## If entities exist but have no values

Open the device's ESPHome logs immediately after boot and check these lines in order:

1. The I2C scan must show a device at `0x76` (or `0x77` if selected with SDO).
2. The BME68x component must not report `Communication failed`, `bme68x_init failed`, or `marked as failed`.
3. Temperature, humidity, and pressure should begin updating first. Gas-derived IAQ, static IAQ, estimated CO2, and breath VOC can remain unavailable while BSEC2 starts and stabilizes.

If no I2C address appears, power the sensor from 3.3 V, connect CS to 3.3 V, connect SDO to GND, check that SDA and SCL are not reversed, and power-cycle the ESP32. A software restart may not recover a sensor that started in the wrong bus mode.

## Calibration expectations

IAQ Accuracy normally starts at `Stabilizing`, then progresses through `Uncertain` or `Calibrating` before reaching `Calibrated`. Keep the device powered and exposed to normal clean and occupied-air conditions. Do not judge IAQ performance from the first few minutes.
