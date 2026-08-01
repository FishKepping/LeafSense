# Milestone 3.0 Alpha

## Purpose

Milestone 3.0 Alpha records the first demonstrated end-to-end LeafSense thermal-camera system. It is a checkpoint for working data flow and channel calculations, not a stable user-interface release.

## Demonstrated on hardware

- Live AMG8833 frame acquisition on ESP32 through ESPHome.
- Full thermal image displayed in Home Assistant.
- Six fixed measurement channels exposed as stable entity sets.
- Direct 8×8 pixel masks written from Home Assistant to the ESP32.
- Correct per-channel minimum, maximum, average, and pixel-count results.
- Sensor-wide calibration values written to the ESP32.

## Alpha defects and incomplete work

| Area | Current state | Required outcome |
|---|---|---|
| ROI editor | Buggy interactions and layout | Reliable desktop and touch editing |
| Browser persistence | ROI geometry is lost on refresh | Restore the exact six-channel state |
| Device persistence | Channels reset after ESP32 restart | Versioned, validated persistent storage |
| Calibration UI | Write path works; UI needs refinement | Clear values, validation, save/default feedback |
| Statistics UI | Values work | Clear, compact six-channel presentation |
| Installation | Package/import files exist | Repeatable, tested Device Builder and HA guide |

## Processing contract

```mermaid
flowchart LR
    Raw["AMG8833 frame"] --> Cal["Sensor-wide calibration"]
    Cal --> Correct["Dead-pixel correction"]
    Correct --> Smooth["Temporal and optional spatial filtering"]
    Smooth --> ROI["Six measurement channels"]
    ROI --> Out["Frame packet and HA entities"]
```

Each channel is always addressable as channel 1–6 and may be disabled or contain an arbitrary set of AMG8833 pixels. A disabled channel keeps its entity identities but publishes no usable measurement. Custom channel names currently live in browser storage.

## Exit criteria

Milestone 3.0 can move beyond alpha when the dashboard UI is reliable, ROI configuration survives browser and device restarts, calibration and statistics are understandable, and a clean installation has passed the published Home Assistant and ESPHome Device Builder instructions.

## Following milestone

After the thermal camera package is stable, integrate environmental sensing—beginning with the BME688—and calculate air and leaf VPD using ROI leaf temperatures. Actuator control and AI/predictive assistance remain later phases.
