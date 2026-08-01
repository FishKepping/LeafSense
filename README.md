# 🌿 LeafSense

> Open-source thermal plant monitoring for ESPHome and Home Assistant.

LeafSense is a modular ESP32 plant-monitoring platform. It currently combines an AMG8833 8 × 8 thermal sensor, ESPHome, and a custom Home Assistant card that displays a live thermal image and sends editable measurement regions to the ESP32.

## Project status

**Version:** `3.0.0-alpha.1`

**Milestone:** `3.0 Alpha — end-to-end thermal camera prototype`

**Status:** Working hardware alpha with known dashboard and persistence issues

The complete data path has been demonstrated on hardware: the AMG8833 frame reaches Home Assistant, the thermal image renders live, six stable measurement channels appear, ROI definitions can be written to the ESP32, and per-channel minimum, maximum, average, and pixel-count statistics update.

This is not yet a polished installation release. The ROI editor is buggy, ROI geometry is lost when the browser refreshes and after the ESP32 restarts, and the calibration and channel-statistics interfaces need refinement.

## What works now

- ESP32/ESPHome acquisition from the Panasonic AMG8833.
- Full 8 × 8 frame transport using a versioned Base64 packet with CRC32.
- Sensor-wide gain and offset calibration before channel processing.
- Calibration values written to the ESP32; explicit save and restore-default controls.
- Dead-pixel correction, temporal smoothing, and optional spatial filtering.
- Six fixed measurement channels, each disabled, rectangle, or polygon.
- ESP32 calculation of channel minimum, maximum, average, and pixel count.
- Stable Home Assistant entities for all six channels.
- Live thermal rendering and ROI overlays in a custom Home Assistant card.
- Runtime ROI updates through ESPHome actions without editing YAML or reflashing.
- Native CMake/Catch2 tests for the reusable core and driver.

## Known Milestone 3.0 Alpha limitations

- ROI creation and editing controls remain unreliable and need UI work.
- ROI geometry is not restored after a browser refresh.
- ROI geometry is not persisted by the ESP32 and returns to disabled after a device restart.
- Calibration writes reach the ESP32, but the card's calibration workflow and feedback need improvement.
- Channel statistics are correct, but their dashboard presentation needs improvement.
- The install path and Home Assistant/ESPHome package are still being validated and documented for repeatable use.

## Current architecture

```mermaid
flowchart LR
    Sensor["AMG8833"] --> Driver["Driver and recovery"]
    Driver --> Process["Calibration and filtering"]
    Process --> Channels["Six measurement channels"]
    Channels --> Packet["Frame packet and entities"]
    Packet --> HA["Home Assistant thermal card"]
```

All thermal calculations remain in Celsius. The dashboard may convert values for display. Calibration is sensor-wide and is applied before the six measurement channels are evaluated.

## Next major goal

The next phase is to turn the alpha prototype into a repeatable Home Assistant experience:

1. Repair and simplify the thermal-card ROI editor.
2. Persist and restore ROI geometry reliably.
3. Improve calibration controls, feedback, and validation.
4. Improve the presentation of six-channel statistics.
5. Test a clean installation from ESPHome Device Builder and Home Assistant.
6. Publish verified wiring, ESPHome package, dashboard installation, upgrade, and troubleshooting instructions.

After that foundation is stable, LeafSense will expand into plant-specific environmental monitoring. The planned next sensor is the BME688 for air temperature, humidity, pressure, and VOC/gas measurements. CO₂ will only be described as estimated when derived from gas sensing; a dedicated CO₂ sensor can be added if true CO₂ measurement is required. LeafSense will calculate VPD using air conditions and ROI leaf temperature as the leaf-temperature offset, then display those results alongside the thermal image.

Fans and other actuators, followed by guarded automation or AI-assisted control, are later phases.

## Repository layout

```text
LeafSense/
├── components/             Git-importable ESPHome component
├── packages/               Reusable ESPHome package
├── leafsense-core/         Platform-independent processing and ROI logic
├── drivers/amg8833/        AMG8833 driver, snapshots, health and recovery
├── firmware/esphome/       ESPHome integration sources
├── homeassistant/          Thermal card, dashboard YAML and browser tests
├── examples/esphome/       Example and import configurations
├── docs/                   Architecture, roadmap and developer guides
└── leafsense-amg8833.yaml  ESPHome Device Builder import manifest
```

## Native build and test

Requirements: CMake 3.20+, a C++17 compiler, and Git.

```bash
cmake -S . -B build -DLEAFSENSE_BUILD_TESTS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

For Ninja or another single-configuration generator, omit `-C Debug` from `ctest`.

## Documentation

- [Milestone 3.0 Alpha status](docs/Milestone_3_0_Alpha.md)
- [Home Assistant dashboard](homeassistant/README.md)
- [Architecture](docs/Architecture.md)
- [Roadmap](docs/Roadmap.md)
- [Development guide](docs/Development.md)
- [Testing guide](docs/Testing.md)
- [AMG8833 driver](docs/Driver_AMG8833.md)
- [Telemetry](docs/Telemetry.md)
- [Project charter](docs/PROJECT_CHARTER.md)

## Contributing and licence

Contributions, hardware testing, and documentation improvements are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

LeafSense is released under the [MIT License](LICENSE).
