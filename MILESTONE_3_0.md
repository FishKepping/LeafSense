# LeafSense Milestone 3.0 — Git-importable ESPHome integration

This package was rebuilt from the attached up-to-date Milestone 2.7 repository snapshot and the uploaded root `CMakeLists.txt`.

## Included

- Standard Git external component at `components/leafsense_amg8833/`
- Remote package at `packages/leafsense_amg8833.yaml`
- Device Builder import file at `leafsense-amg8833.yaml`
- Local secrets wrapper example
- Full frame Base64 text sensor
- Sensor-wide calibration with explicit save/default controls
- Milestone 2.9 processing before measurement channels
- Six fixed channel result sets
- Existing six-channel runtime API
- Milestone 2.7 card updated to call the existing multi-action API

## Processing order

AMG8833 capture → calibration → dead-pixel correction → temporal smoothing → optional spatial median → six measurement channels → frame packet → Home Assistant.

## ESPHome Device Builder import

After committing these files to `main`, import:

`github://FishKepping/LeafSense/leafsense-amg8833.yaml@main`

The configuration intentionally sets `api.custom_services: true` because the existing `MeasurementChannelRuntimeApi` registers user-defined actions.

## Dashboard action prefix

The included card uses:

`service_prefix: esphome.leafsense_amg8833`

If Home Assistant creates a different prefix, open Developer Tools → Actions, locate `leafsense_channel_disable`, and replace the prefix in the card YAML. The action suffixes are fixed by the existing runtime API.

## Known limitation, not silently filled in

The attached native controller does not persist ROI geometry. Calibration is persistent when the Save Calibration button is pressed; channel shapes return to disabled after an ESP32 restart. This package does not invent a persistence format that is absent from the supplied source.

## Validation

1. Run native build and CTest locally.
2. Commit and push this package.
3. Import it in ESPHome Device Builder.
4. Validate, compile and flash.
5. Confirm `text_sensor.leafsense_thermal_frame` changes every update.
6. Confirm all six channel entity sets exist.
7. Confirm the six ROI actions appear in Home Assistant.
8. Test rectangle, polygon and rotated rectangle editing.
9. Save calibration, restart, and confirm it reloads.
