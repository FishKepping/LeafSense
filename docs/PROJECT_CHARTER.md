# LeafSense Project Charter

## Mission

Build a reliable, maintainable, and open-source plant-monitoring platform based on ESP32, ESPHome, Home Assistant, thermal imaging, environmental sensing, and safe automation.

## Vision

LeafSense should allow people to monitor plant and growing-environment temperatures without specialist knowledge of thermal cameras or embedded programming.

The intended finished experience combines:

- ESP32 sensor nodes managed through ESPHome.
- Thermal and environmental measurements.
- A clear Home Assistant dashboard.
- Interactive regions of interest drawn over a thermal image.
- Region minimum, maximum, and average temperatures.
- Safe automation of environmental controls.
- Future local prediction or decision support.

The system should remain approachable for a single user while being reusable by others.

## Primary platform

ESPHome is the primary deployment platform.

Native C++ modules should remain reusable when that improves testing and design without making the ESPHome component harder to build or use.

## Current release goal

### Version 0.1 — Seed

Deliver a stable thermal-sensing foundation capable of:

- Reading frames from an AMG8833.
- Decoding and optionally filtering thermal data.
- Reporting sensor status and structured errors.
- Recovering from repeated communication failures.
- Reading hardware interrupt state.
- Producing complete snapshots.
- Projecting snapshots into ESPHome-ready telemetry.
- Running under ESPHome on an ESP32.
- Publishing reliable whole-frame measurements and diagnostics to Home Assistant.
- Providing documented setup and a working example.

Interactive regions and predictive controls are not part of Seed.

## Users

LeafSense is built primarily for its maintainer's own plant-monitoring system.

It should also be understandable, installable, and modifiable by:

- Home Assistant and ESPHome users.
- Greenhouse and indoor-growing hobbyists.
- Embedded developers.
- Open-source contributors.
- People experimenting with low-resolution thermal sensing.

## Development principles

1. Foundation before features.
2. Small, focused changes.
3. Test behavior early.
4. Keep documentation synchronized with code.
5. Avoid unnecessary complexity.
6. Keep modules independent.
7. Use dependency injection at hardware boundaries.
8. Keep reusable code independent of ESPHome.
9. Prefer fixed-size storage in embedded runtime paths.
10. Make unavailable and failed data explicit.
11. Preserve published APIs where practical.
12. Every commit should leave the project buildable.
13. Validate hardware behavior before declaring a release stable.

## Architectural commitments

- C++17 native core.
- CMake native build.
- Unit tests with Catch2.
- ESPHome as the primary device platform.
- Home Assistant as the primary user experience.
- MIT licensing.
- No cloud requirement for core operation.
- No model-controlled actuation without deterministic safeguards.

## Scope control

Features outside the active milestone are recorded in the roadmap and deferred until the current foundation is complete.

This protects the project from prematurely adding:

- Complex dashboards before reliable data exists.
- Region editing before frame transport is proven.
- Predictive models before clean historical data exists.
- Multiple thermal sensors before the AMG8833 path is stable.
- Advanced automation before stale-data and failure behavior are safe.

## Definition of done

A feature is complete when applicable:

- Code compiles in a clean build.
- Relevant native unit tests pass.
- Failure paths are tested.
- Compiler warnings are resolved.
- Public behavior is documented.
- The roadmap and changelog are updated.
- ESP32 hardware behavior is tested.
- Example configuration exists.
- User-visible unavailable and failure states are sensible.
- No unrelated regression is introduced.

## Long-term success

LeafSense succeeds when a user can install an ESP32 thermal node, view a thermal image in Home Assistant, draw useful plant regions, monitor their temperatures, and build dependable environmental controls without needing to understand the internal thermal-processing pipeline.
