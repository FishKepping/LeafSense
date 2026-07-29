# LeafSense Home Assistant dashboard

Milestone 2.5 adds a Lovelace custom card that:

- decodes the Milestone 2.3 Base64/CRC32 thermal frame packet;
- renders the live AMG8833 8×8 frame as a colour thermal image;
- supports automatic or fixed temperature scales;
- overlays all six stable measurement channels;
- edits rectangles and polygons in sensor coordinates `0.0–8.0`;
- sends runtime channel geometry through a Home Assistant service.

## Installation

1. Copy `homeassistant/www/leafsense-thermal-card.js` to Home Assistant:
   `/config/www/leafsense-thermal-card.js`
2. In Home Assistant, open **Settings → Dashboards → Resources**.
3. Add `/local/leafsense-thermal-card.js` as a **JavaScript module**.
4. Copy the example dashboard card from
   `dashboards/leafsense-thermal-view.yaml` and change the entity ID if needed.
5. Refresh the browser with `Ctrl+F5`.

## Required thermal frame entity

The configured entity must expose the complete Milestone 2.3 Base64 packet as
its state. Default example:

`sensor.leafsense_thermal_frame`

## Runtime ROI service contract

The card defaults to:

`esphome.leafsense_set_measurement_channel`

Service data:

```yaml
channel: 1
# disabled, rectangle, or polygon
type: rectangle
# JSON because ESPHome custom API services accept scalar arguments reliably
points: '[{"x":1.0,"y":1.0},{"x":6.0,"y":5.0}]'
```

The firmware/API adapter must validate:

- channel is 1–6;
- coordinates are finite and clamped to 0.0–8.0;
- rectangles have exactly two points;
- polygons have at least three points and obey the channel vertex limit;
- disabled channels ignore points and publish unavailable statistics.

## Editing

- Choose Channel 1–6.
- Select Rectangle, Polygon, or Disabled.
- Rectangle: drag from one corner to the opposite corner.
- Polygon: click each vertex; double-click or press **Apply ROI** when complete.
- **Clear** disables the selected local overlay; press **Apply ROI** to send it.

## Browser test

Open `homeassistant/tests/leafsense-thermal-card.test.html` in a browser. It
checks CRC32, palette mapping, and custom-card registration. Expected result:

`ALL TESTS PASSED`

## Milestone boundary

This milestone delivers the Home Assistant visual/editor layer and its service
contract. The ESPHome adapter that publishes the packet entity and implements
the service belongs in the next firmware integration step, because it must be
matched to the current local ESPHome component rather than guessed from the
older public repository tree.
