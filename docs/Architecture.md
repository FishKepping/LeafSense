# Architecture

LeafSense consists of four logical layers.

Hardware

↓

Driver

↓

Core

↓

Platform

The driver acquires raw thermal data.

The core processes immutable thermal frames.

The platform exposes processed information to ESPHome.

Future user interfaces, dashboards and Home Assistant integrations communicate only with the platform layer.