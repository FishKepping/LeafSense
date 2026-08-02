# Changelog

All notable changes to LeafSense will be documented in this file.

The format is based on Keep a Changelog principles. The project has not yet made its first tagged release.

## [Unreleased]

### Milestone 3.0 Alpha checkpoint

- Demonstrated live AMG8833 thermal rendering in Home Assistant.
- Added a versioned Base64/CRC32 full-frame transport.
- Added six fixed runtime measurement channels supporting disabled or arbitrary 8×8 pixel-mask geometry.
- Replaced the Home Assistant shape editor with direct click/drag pixel-mask editing and browser-persisted custom ROI names.
- Published minimum, maximum, average, and pixel-count entities for every channel.
- Added sensor-wide gain/offset calibration before measurement-channel processing, with explicit save and restore-default controls.
- Added a custom thermal card with ROI editing, channel statistics, diagnostics, display settings, and calibration controls.
- Confirmed the Home Assistant dashboard on hardware as a working alpha checkpoint.
- Added browser-refresh persistence for all six ROI masks and custom names.
- Added full-frame minimum, average, and maximum display plus per-ROI colour feedback.
- Added automatic ESP32 flash persistence for calibration gain and offset.
- Added a Git-importable ESPHome component, reusable package, and Device Builder import manifest.
- Recorded remaining alpha limitations: no ESP32-side ROI persistence, browser-local ROI names/masks, minor UI polish, and an unverified clean-install guide.
- Set the next release objective to a tested Home Assistant dashboard and ESPHome Device Builder/package installation path.
- Planned BME688 environmental sensing and VPD calculations using ROI leaf temperature after the thermal/ROI release is stable.

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
