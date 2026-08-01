# LeafSense Home Assistant dashboard — Milestone 3.0 Alpha

The custom `leafsense-thermal-card` is a working hardware alpha. It decodes the live AMG8833 frame, draws a thermal image, displays six fixed measurement channels, and sends rectangle or polygon channel definitions to the ESP32.

## Confirmed working

- Live Base64/CRC32 thermal frame decoding and rendering.
- Six stable channel entity sets.
- ESP32-generated minimum, maximum, average, and pixel-count statistics.
- Rectangle and polygon geometry sent through ESPHome actions.
- Sensor-wide gain and offset written through Home Assistant entities.
- Explicit calibration save and restore-default buttons.
- Celsius/Fahrenheit display conversion, palettes, scaling, interpolation, grid, legend, and labels.

## Known alpha limitations

- ROI editing is buggy and needs interaction and layout improvements.
- ROI geometry exists only in the card's runtime state; refreshing the browser loses it.
- The ESP32 does not yet persist channel geometry; restarting it disables the channels.
- Writing calibration works, but entity discovery, value synchronisation, feedback, and layout need refinement.
- Statistics work, but the six-channel display needs clearer visual grouping and better mobile behaviour.
- The installation below is provisional until it has been repeated from a clean Home Assistant and ESPHome setup.

## Provisional installation

1. Install and flash the LeafSense ESPHome configuration. The Device Builder import target is:

   ```text
   github://FishKepping/LeafSense/leafsense-amg8833.yaml@main
   ```

2. Copy `www/leafsense-thermal-card.js` to `/config/www/leafsense-thermal-card.js`.
3. In Home Assistant, open **Settings → Dashboards → Resources**.
4. Add `/local/leafsense-thermal-card.js` as a JavaScript module.
5. Add the card YAML from `dashboards/leafsense-thermal-view.yaml` and correct entity/action prefixes if necessary.
6. Hard-refresh the browser with `Ctrl+F5`.

Do not treat these as final public installation instructions yet. Clean-install testing and a complete Device Builder/package guide are the next milestone.

## Card configuration

```yaml
type: custom:leafsense-thermal-card
title: LeafSense Thermal View
entity: text_sensor.leafsense_thermal_frame
channel: 1
service_prefix: esphome.leafsense_amg8833
```

If Home Assistant assigns different entity IDs, configure `channel_entities` or `calibration_entities` explicitly. If it assigns a different ESPHome action prefix, open **Developer Tools → Actions**, find `leafsense_channel_disable`, and update `service_prefix`.

## Runtime action contract

The card uses separate ESPHome actions:

- `leafsense_channel_disable`
- `leafsense_channel_set_rectangle`
- `leafsense_channel_polygon_begin`
- `leafsense_channel_polygon_point`
- `leafsense_channel_polygon_commit`
- `leafsense_channel_polygon_cancel`

Coordinates use native sensor space from `0.0` through `8.0`. Rotated rectangles are transmitted as four-point polygons.

## Calibration behaviour

Calibration is applied to the entire AMG8833 before dead-pixel correction, smoothing, and channel calculations:

```text
calibrated temperature = (raw temperature × gain) + offset
```

Changing gain or offset updates the running ESP32 state. Press **Save Calibration** to persist it across a restart. **Restore Calibration Defaults** restores gain `1.0` and offset `0.0`.

Calibration persistence does not imply ROI persistence; they are separate systems.

## Validation

The browser test is at `tests/leafsense-thermal-card.test.html`. It covers packet CRC, colour output, card registration, temperature conversion, rectangle conversion, and rotation geometry. It does not yet provide full automated coverage of pointer editing, Home Assistant action calls, persistence, reconnection, or mobile layouts.

Before a stable release, validate:

- Browser refresh and Home Assistant restart.
- ESP32 restart and power loss.
- All six rectangle/polygon/disabled channels.
- Calibration apply, save, restore, and reboot behaviour.
- Desktop, tablet, and mobile pointer/touch editing.
- A clean Device Builder and dashboard installation.
