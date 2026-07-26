# LeafSense Roadmap

## Status legend

| Status | Meaning |
|---|---|
| ✅ Completed | Implemented and covered by the current native test suite |
| 🚧 In progress | Active development |
| 🗓️ Planned | Accepted direction, not yet implemented |
| 💡 Future | Long-term idea requiring design and validation |

## Current release

**Release:** `v0.1 — Seed`  
**Current completed milestone:** `1.8 — Publication-ready telemetry`  
**Current phase:** Documentation refresh, followed by ESPHome integration

## 🌱 Seed — Reliable thermal foundation

### Completed

| Milestone | Status | Description |
|---|---:|---|
| 1.1 — Core frame model | ✅ | Added the fixed-size 8 × 8 `ThermalFrame` and frame metadata. |
| 1.2 — AMG8833 decoding | ✅ | Added platform-independent decoding of thermistor and pixel register values. |
| 1.3 — Thermal processing | ✅ | Added optional spatial and exponential temporal filtering. |
| 1.4 — AMG8833 driver | ✅ | Added bus abstraction, initialization, acquisition, status, and structured errors. |
| 1.5 — Automatic recovery | ✅ | Added health counters, failure thresholds, and controlled reinitialization. |
| 1.6 — Interrupt support | ✅ | Added interrupt configuration, status handling, and 8 × 8 interrupt-map decoding. |
| 1.7 — Unified snapshots | ✅ | Combined frame data, summary statistics, diagnostics, recovery, and optional interrupt data. |
| 1.8 — Telemetry projection | ✅ | Flattened snapshots into values and flags suitable for ESPHome publication. |
| Documentation foundation | 🚧 | Update architecture, driver, telemetry, testing, and developer documentation. |

Milestone names before 1.8 summarize the implemented development sequence. They should be kept aligned with commit history if formal release tags are introduced.

### Next Seed milestones

| Milestone | Status | Description |
|---|---:|---|
| 1.9 — ESPHome bus adapter | 🗓️ | Connect `Amg8833Bus` to ESPHome's I²C facilities. |
| 1.10 — ESPHome component lifecycle | 🗓️ | Add configuration validation, setup, update, and failure reporting. |
| 1.11 — Base Home Assistant entities | 🗓️ | Publish whole-frame min, max, average, thermistor, health, and diagnostics. |
| 1.12 — Hardware validation | 🗓️ | Validate readings, recovery, timing, and stability on an ESP32 and physical AMG8833. |
| 1.13 — Example configuration | 🗓️ | Provide a documented ESPHome YAML example and wiring guide. |
| 1.14 — Seed release hardening | 🗓️ | Resolve warnings, verify clean builds, finalize docs, and tag v0.1. |

### Seed definition of done

Seed is complete when:

- Native builds and tests pass on supported development environments.
- The AMG8833 runs reliably on an ESP32 through ESPHome.
- Whole-frame temperatures and diagnostics appear in Home Assistant.
- Automatic recovery is verified against real communication failures.
- Setup, wiring, configuration, and troubleshooting are documented.
- A working example configuration is provided.
- The code and documentation are released under MIT.

## 🌿 Sprout — Stable processing and environmental context

**Goal:** Move from basic thermal acquisition to a dependable monitoring node.

| Feature | Status | Description |
|---|---:|---|
| Calibration support | 🗓️ | Add configurable offset and validation against a reference. |
| Processing configuration | 🗓️ | Expose filtering and update behavior safely through ESPHome YAML. |
| Expanded diagnostics | 🗓️ | Add timing, stale-data, and processing-health diagnostics. |
| Environmental sensors | 🗓️ | Integrate temperature, humidity, pressure, light, soil, or related sensors through modular ESPHome configuration. |
| Historical dashboard | 🗓️ | Provide sensible Home Assistant history and trend views. |
| Simulator improvements | 🗓️ | Feed repeatable thermal scenes into core and UI development. |

## 🍃 Leaf — Regions of interest

**Goal:** Let users define meaningful plant regions on a thermal image.

| Feature | Status | Description |
|---|---:|---|
| Region data model | 🗓️ | Define named regions independently of the UI. |
| Rectangle regions | 🗓️ | Support movable and resizable rectangles. |
| Polygon regions | 🗓️ | Support draggable polygon vertices. |
| Region statistics | 🗓️ | Calculate minimum, maximum, average, and valid-pixel counts. |
| Coordinate mapping | 🗓️ | Map displayed/interpolated image coordinates to the native 8 × 8 grid. |
| Region persistence | 🗓️ | Save and restore region definitions. |
| Region entities | 🗓️ | Publish each region's statistics to Home Assistant. |
| Region templates | 🗓️ | Export, import, and reuse region layouts. |

## 🌳 Canopy — Integrated Home Assistant experience

**Goal:** Deliver an easy-to-use dashboard rather than a collection of raw entities.

| Feature | Status | Description |
|---|---:|---|
| Live thermal display | 🗓️ | Render the thermal frame clearly in Home Assistant. |
| Interactive ROI editor | 🗓️ | Draw, drag, resize, rename, enable, and disable regions. |
| Automation controls | 🗓️ | Link region temperatures and environmental conditions to controls. |
| Device management | 🗓️ | Make multiple ESP32 nodes understandable and maintainable. |
| Presets and onboarding | 🗓️ | Provide guided setup for common greenhouse and indoor-growing uses. |
| Accessibility and mobile layout | 🗓️ | Ensure the dashboard remains usable across devices. |

## 🌸 Bloom — Advanced automation

**Goal:** Use thermal regions and environmental data to make robust control decisions.

| Feature | Status | Description |
|---|---:|---|
| Rule templates | 🗓️ | Provide safe automation examples for fans, pumps, shades, heaters, and alerts. |
| Multi-sensor fusion | 🗓️ | Combine thermal, air, soil, light, and actuator state. |
| Trend and anomaly detection | 🗓️ | Identify developing stress rather than only threshold crossings. |
| Control safeguards | 🗓️ | Add limits, cooldowns, stale-data checks, manual override, and failure-safe behavior. |
| Multi-node coordination | 🗓️ | Combine measurements from several ESP32 nodes. |

## 🍅 Harvest — Predictive assistance

**Goal:** Explore useful prediction while retaining deterministic safeguards.

| Feature | Status | Description |
|---|---:|---|
| Data collection format | 💡 | Record clean, explainable environmental and control history. |
| Feature engineering | 💡 | Define inputs suitable for a compact model. |
| Environmental forecasting | 💡 | Predict temperature, humidity, or plant-stress trends. |
| Control recommendation | 💡 | Recommend bounded actions to Home Assistant. |
| Edge deployment study | 💡 | Evaluate whether useful inference can run on an ESP32-class target or should run locally in Home Assistant. |
| Safety envelope | 💡 | Ensure predictions cannot bypass deterministic safety constraints. |

No specific LLM or model architecture has been selected. The appropriate approach may be a small time-series model, rules plus anomaly detection, a compact neural network, or another local model rather than a language model.

## Out of scope until the foundation is stable

- Autonomous model-controlled actuators without deterministic safeguards.
- Cloud-only operation.
- Hidden or proprietary data formats.
- Tight coupling of the driver to a single dashboard.
- Supporting many thermal sensor types before the AMG8833 path is proven.
