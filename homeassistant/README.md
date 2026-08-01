# LeafSense Home Assistant dashboard — Milestone 3.0 Alpha

The custom `leafsense-thermal-card` is a working hardware alpha. It decodes the live AMG8833 frame, draws a thermal image, displays six fixed measurement channels, and sends direct 8×8 pixel-mask channel definitions to the ESP32.

## Confirmed working

- Live Base64/CRC32 thermal frame decoding and rendering.
- Six stable channel entity sets.
- ESP32-generated minimum, maximum, average, and pixel-count statistics.
- Click-toggle and drag paint/erase pixel-mask editing.
- Browser-persisted custom ROI names shown on tabs and the thermal image.
- Sensor-wide gain and offset written through Home Assistant entities.
- Explicit calibration save and restore-default buttons.
- Celsius/Fahrenheit display conversion, palettes, scaling, interpolation, grid, legend, and labels.

## Known alpha limitations

- ROI editing is still alpha and needs hardware testing across desktop and mobile browsers.
- ROI geometry is stored per LeafSense device in the current browser. It survives a browser refresh, but it does not automatically follow the user to another browser or Home Assistant companion device.
- The ESP32 does not yet persist channel geometry; restarting it disables the channels.
- Writing calibration works, but automatic entity discovery still depends on Home Assistant's generated entity IDs.
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
- `leafsense_channel_set_pixel_mask`

The pixel-mask action accepts `channel` plus `row_0` through `row_7`. Each row is an integer from `0` to `255`; bit 0 is the leftmost pixel and bit 7 is the rightmost pixel.

After a successful ESP32 action, the card stores all six ROI definitions in browser local storage using the thermal-frame entity and ESPHome action prefix as the device identity. A failed action restores only the affected channel to its last successfully saved geometry.

## Calibration behaviour

Calibration is applied to the entire AMG8833 before dead-pixel correction, smoothing, and channel calculations:

```text
calibrated temperature = (raw temperature × gain) + offset
```

Changing gain or offset updates the running ESP32 state. Press **Save Calibration** to persist it across a restart. **Restore Calibration Defaults** restores gain `1.0` and offset `0.0`.

Calibration persistence does not imply ROI persistence; they are separate systems.

## Validation

The browser test is at `tests/leafsense-thermal-card.test.html`. It covers packet CRC, colour output, card registration, temperature conversion, device-scoped ROI storage keys, and stored pixel-mask/name validation. It does not yet provide full automated coverage of pointer editing, Home Assistant action calls, reconnection, or mobile layouts.

Before a stable release, validate:

- Browser refresh and Home Assistant restart.
- ESP32 restart and power loss.
- All six pixel-mask/disabled channels.
- Calibration apply, save, restore, and reboot behaviour.
- Desktop, tablet, and mobile pointer/touch editing.
- A clean Device Builder and dashboard installation.
