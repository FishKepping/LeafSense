# Changelog

All notable changes to LeafSense will be documented in this file.

The format is based on Keep a Changelog principles. The project has not yet made its first tagged release.

## [Unreleased]

### Added

- Platform-independent C++17 thermal core.
- Fixed-size 8 × 8 `ThermalFrame`.
- AMG8833 pixel and thermistor decoding.
- Optional mean and median spatial filtering.
- Optional exponential temporal filtering.
- Dependency-injected `Amg8833Bus` interface.
- AMG8833 initialization, status decoding, and frame acquisition.
- Structured driver errors and health reporting.
- Automatic recovery after repeated failures.
- Interrupt configuration and 8 × 8 interrupt-map decoding.
- Unified `Amg8833Snapshot` capture model.
- Whole-frame minimum, maximum, average, thermistor, and valid-pixel summary.
- `Amg8833Telemetry` publication contract.
- Stateless `Amg8833TelemetryProjector`.
- Native CMake build and Catch2 test suites.
- Expanded architecture, driver, testing, development, telemetry, roadmap, and Home Assistant vision documentation.

### Changed

- Clarified ESPHome as the primary deployment platform while retaining reusable native modules.
- Expanded the Seed roadmap into completed, in-progress, and planned milestones.
- Defined interactive Home Assistant regions of interest as a later roadmap phase.
- Defined future predictive assistance as a guarded, post-foundation research area.
- Expanded contribution and definition-of-done requirements.

### Fixed

- Test compatibility and build diagnostics encountered during snapshot and telemetry milestone development.

## [0.1.0] — Unreleased

### Planned

- ESPHome I²C bus adapter.
- ESPHome component lifecycle.
- Whole-frame Home Assistant measurement and diagnostic entities.
- ESP32 and physical AMG8833 validation.
- Wiring and YAML examples.
