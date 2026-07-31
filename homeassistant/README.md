# LeafSense Home Assistant dashboard — Milestone 2.7

Milestone 2.7 upgrades the thermal card into an intuitive ROI editor while retaining the Milestone 2.5 packet and ESPHome service contracts.

## Features

- Decodes the Base64/CRC32 AMG8833 packet.
- Displays six stable measurement channels.
- Rectangle drag drawing.
- Polygon point drawing.
- Move and vertex editing.
- Rotation handles for rectangles and polygons.
- Configurable 5°, 15°, 30° or 45° rotation snapping.
- Undo, redo, duplicate, delete, save and cancel.
- Automatic, Celsius or Fahrenheit display.
- Automatic or fixed temperature scaling.
- Thermal, iron and greyscale palettes.
- Smooth or pixelated rendering.
- Optional sensor grid, legend and ROI labels.
- Slide-out display, scale, ROI, diagnostics and calibration settings.
- Browser-local preference persistence.

LeafSense firmware, packets, calibration and ROI calculations remain Celsius internally. Fahrenheit conversion is display-only.

## Installation

1. Copy `www/leafsense-thermal-card.js` to `/config/www/leafsense-thermal-card.js`.
2. In Home Assistant, open **Settings → Dashboards → Resources**.
3. Add `/local/leafsense-thermal-card.js` as a JavaScript module.
4. Copy `dashboards/leafsense-thermal-view.yaml` into your dashboard and correct the entity/service IDs if necessary.
5. Hard-refresh with `Ctrl+F5`.

## Editing workflow

1. Press the pencil button.
2. Select Channel 1–6.
3. Choose **Rectangle** and drag, or **Polygon** and click each vertex.
4. Switch to **Select**.
5. Drag inside a shape to move it.
6. Drag a white vertex to reshape it.
7. Drag the circular handle above the ROI to rotate it.
8. Press **Save** to send the channel to ESPHome or **Cancel** to restore the editor snapshot.

An unrotated rectangle is sent using the existing two-point `rectangle` contract. Once rotated, it is sent as a four-point `polygon`; this avoids adding a new firmware geometry type.

## Service contract

Default service:

`esphome.leafsense_set_measurement_channel`

```yaml
channel: 1
type: rectangle  # disabled, rectangle or polygon
points: '[{"x":1.0,"y":1.0},{"x":6.0,"y":5.0}]'
```

All coordinates remain in sensor space from `0.0` through `8.0`.

## Settings

The gear panel contains:

- **Display:** unit, palette, interpolation, grid, legend and labels.
- **Temperature scale:** automatic/fixed and display-unit min/max.
- **ROI editor:** grid snapping, rotation snapping and coordinates.
- **Diagnostics:** frame sequence, protocol, CRC, calibration revision and dimensions.
- **Calibration:** explains the sensor-wide firmware boundary and links conceptually to the existing calibration dashboard.

Settings are stored in browser `localStorage`; they do not alter firmware values.

## Tests

Open `tests/leafsense-thermal-card.test.html` in a browser. Expected result:

`ALL TESTS PASSED`

The test covers CRC32, palette output, card registration, Celsius/Fahrenheit conversion, rectangle conversion and rotation geometry.

## Hardware validation

Use `HARDWARE_VALIDATION_2_7A.md` and complete the 24-hour stability checkpoint before Milestone 2.8.
