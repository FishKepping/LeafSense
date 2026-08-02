# LeafSense Roadmap

## Status legend

| Status | Meaning |
|---|---|
| ✅ Working | Demonstrated in the current hardware path |
| 🧪 Alpha | Implemented but still needs UI, reliability, or installation testing |
| 🚧 Next | The next committed development phase |
| 🗓️ Planned | Accepted direction after the next phase |
| 💡 Future | Long-term work requiring design and validation |

## Current release: Milestone 3.0 Alpha

**Software version:** `3.0.0-alpha.1`

**Checkpoint:** End-to-end AMG8833, ESPHome, and Home Assistant prototype

| Capability | Status | Current result |
|---|---:|---|
| Live thermal frame | ✅ | The AMG8833 image updates in the Home Assistant card. |
| Six stable channels | ✅ | All channel entities exist; disabled channels remain unavailable. |
| Direct pixel-mask channel processing | 🧪 | Arbitrary 8×8 cell selections are sent to the ESP32 and retain the existing statistics entities. |
| Channel min/max/average/pixel count | ✅ | Calculated on the ESP32 and published to Home Assistant. |
| Sensor-wide calibration | ✅ | Gain and offset changes reach the ESP32 and restore from flash after restart. |
| ROI editor | ✅ | Hardware-tested click-toggle and drag paint/erase editing works for six pixel-mask channels. |
| Browser ROI restoration | ✅ | All six masks and custom names restore after refresh in the same browser. |
| ESP32 ROI restoration | 🧪 | Channels still reset after an ESP32 restart. |
| Statistics UI | ✅ | Full-frame and six-channel statistics are displayed; minor polish remains. |
| Device Builder/package installation | 🧪 | Files exist but the clean user installation path is not yet release-tested. |

## Immediate next phase: Home Assistant release candidate

This is the project's next major goal.

1. Complete minor dashboard usability, touch, layout, and visual polish.
2. Add ESP32-side channel persistence so a device restart does not silently disable configured ROIs.
3. Define safe synchronisation between ESP32 state and browser-local masks/names.
4. Test a clean install through ESPHome Device Builder and Home Assistant.
5. Publish a working package plus verified wiring, secrets, installation, dashboard, upgrade, rollback, and troubleshooting instructions.
6. Complete browser, native, ESPHome compile, hardware restart, and long-duration validation.

The release criterion is a new user being able to install a thermal camera, draw ROIs, restart Home Assistant/browser/ESP32, and recover a functioning configuration by following the published guide.

## Plant sensing phase: environmental context and VPD

After the thermal/ROI installation path is stable:

| Feature | Status | Description |
|---|---:|---|
| BME688 integration | 🗓️ | Add modular air temperature, humidity, pressure, and VOC/gas sensing. |
| CO₂ strategy | 🗓️ | Clearly distinguish BME688-derived estimates from true CO₂ measurements; evaluate a dedicated CO₂ sensor where accuracy matters. |
| Air VPD | 🗓️ | Calculate VPD from measured air temperature and relative humidity. |
| Leaf VPD | 🗓️ | Use each ROI's leaf temperature as the leaf-temperature input/offset. |
| Thermal overlay | 🗓️ | Show environmental readings and VPD with the thermal image and channel statistics. |
| History and diagnostics | 🗓️ | Add trends, stale-data detection, sensor health, and cross-sensor validation. |

## Control phase

| Feature | Status | Description |
|---|---:|---|
| Fans and other actuators | 💡 | Integrate controllable outputs through ESPHome/Home Assistant. |
| Deterministic automation | 💡 | Add thresholds, hysteresis, cooldowns, manual overrides, and failure-safe behaviour. |
| Multi-sensor decisions | 💡 | Combine leaf VPD, air conditions, ROI trends, and actuator state. |

## Intelligence phase

AI or predictive assistance is a future layer, not part of Milestone 3.0. It may provide trend prediction, anomaly detection, or bounded recommendations after trustworthy data collection and safe deterministic controls exist. No autonomous model may bypass availability checks, limits, cooldowns, or manual override.
