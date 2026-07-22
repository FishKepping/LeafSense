# 🌿 LeafSense

> Open-source thermal plant monitoring for ESPHome and Home Assistant.

LeafSense is a modular thermal sensing platform designed for greenhouse automation, precision agriculture, and environmental monitoring.

The project combines ESPHome, Home Assistant, and thermal imaging to provide accurate leaf temperature measurements using configurable plant zones.

The initial hardware target is the Panasonic AMG8833 (Grid-EYE 8×8 thermal camera), with an architecture designed to support additional thermal sensors in the future.

---

## Vision

LeafSense aims to make thermal sensing accessible to everyone.

Rather than requiring users to understand thermal cameras, image processing, or pixel mathematics, LeafSense provides an intuitive platform for measuring and monitoring plant temperatures.

The long-term goal is to become the standard open-source thermal sensing platform for Home Assistant.

---

## Current Status

🚧 **Early Development**

Version: **v0.1 – Seed**

Current focus:

- Build a reliable AMG8833 driver
- Create a reusable thermal processing engine
- Integrate with ESPHome
- Build a solid software architecture

Future features are intentionally deferred until the core platform is stable.

---

## Initial Features

- AMG8833 driver
- ESPHome external component
- Immutable thermal frame architecture
- Plant zone processing
- Polygon geometry engine
- Frame statistics
- Automatic driver recovery
- Unit testing
- Documentation

---

## Project Structure

```
LeafSense
│
├── leafsense-core/
├── leafsense-esphome/
├── leafsense-homeassistant/
├── docs/
├── tests/
├── examples/
└── tools/
```

---

## Development Philosophy

LeafSense follows a few simple principles:

- Build a solid foundation before adding features.
- Keep modules independent and reusable.
- Every commit should leave the project in a working state.
- Documentation grows alongside the code.
- Simplicity is preferred over cleverness.

---

## Roadmap

Current milestone:

🌱 **Seed**

Future milestones:

🌿 Sprout

🍃 Leaf

🌳 Canopy

🌸 Bloom

🍅 Harvest

---

## License

MIT License