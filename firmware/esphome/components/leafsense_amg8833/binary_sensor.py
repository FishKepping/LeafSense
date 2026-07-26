from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import LeafSenseAmg8833Component

CONF_LEAFSENSE_AMG8833_ID = "leafsense_amg8833_id"

CONF_CONNECTED = "connected"
CONF_DRIVER_PROBLEM = "driver_problem"
CONF_FRAME_AVAILABLE = "frame_available"
CONF_OVERFLOW_DETECTED = "overflow_detected"
CONF_INTERRUPT_DETECTED = "interrupt_detected"
CONF_RECOVERY_ACTIVE = "recovery_active"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_LEAFSENSE_AMG8833_ID): cv.use_id(
            LeafSenseAmg8833Component
        ),
        cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_DRIVER_PROBLEM): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_FRAME_AVAILABLE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(
            CONF_OVERFLOW_DETECTED
        ): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(
            CONF_INTERRUPT_DETECTED
        ): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_RECOVERY_ACTIVE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def register_optional_binary_sensor(config, parent, key, setter):
    if key not in config:
        return

    sens = await binary_sensor.new_binary_sensor(config[key])
    cg.add(getattr(parent, setter)(sens))


async def to_code(config):
    parent = await cg.get_variable(
        config[CONF_LEAFSENSE_AMG8833_ID]
    )

    definitions = (
        (CONF_CONNECTED, "set_connected_binary_sensor"),
        (
            CONF_DRIVER_PROBLEM,
            "set_driver_problem_binary_sensor",
        ),
        (
            CONF_FRAME_AVAILABLE,
            "set_frame_available_binary_sensor",
        ),
        (
            CONF_OVERFLOW_DETECTED,
            "set_overflow_detected_binary_sensor",
        ),
        (
            CONF_INTERRUPT_DETECTED,
            "set_interrupt_detected_binary_sensor",
        ),
        (
            CONF_RECOVERY_ACTIVE,
            "set_recovery_active_binary_sensor",
        ),
    )

    for key, setter in definitions:
        await register_optional_binary_sensor(
            config,
            parent,
            key,
            setter,
        )
