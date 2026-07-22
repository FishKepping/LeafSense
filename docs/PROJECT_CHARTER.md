# Project Charter

## Mission

Build a reliable, maintainable, and open-source thermal sensing platform for ESPHome and Home Assistant.

---

## Vision

LeafSense should allow users to monitor plant temperatures without requiring specialist knowledge of thermal imaging.

The software should be approachable for beginners while remaining flexible enough for advanced users and future sensor platforms.

---

## Version 0.1 Goal

Develop a stable thermal sensing foundation capable of:

- Reading frames from an AMG8833.
- Publishing reliable thermal data.
- Providing the core framework for future processing.

Nothing else.

---

## Development Principles

- Foundation before features.
- Small commits.
- Test early.
- Document everything.
- No unnecessary complexity.
- Keep modules independent.
- Maintain backwards compatibility once APIs are published.

---

## Scope Freeze

Features outside Version 0.1 are recorded as backlog items and are not implemented until the current milestone is complete.

---

## Definition of Done

A feature is complete when:

- Code compiles.
- Unit tests pass.
- ESP32 hardware has been tested.
- Documentation has been updated.
- Example configuration exists.