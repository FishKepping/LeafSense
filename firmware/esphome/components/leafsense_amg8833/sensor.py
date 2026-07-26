from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import LeafSenseAmg8833Component

CONF_LEAFSENSE_AMG8833_ID = "leafsense_amg8833_id"

CONF_MINIMUM_TEMPERATURE = "minimum_temperature"
CONF_MAXIMUM_TEMPERATURE = "maximum_temperature"
CONF_AVERAGE_TEMPERATURE = "average_temperature"
CONF_THERMISTOR_TEMPERATURE = "thermistor_temperature"

CONF_FRAME_COUNT = "frame_count"
CONF_VALID_PIXEL_COUNT = "valid_pixel_count"
CONF_ACTIVE_INTERRUPT_PIXEL_COUNT = "active_interrupt_pixel_count"
CONF_CONSECUTIVE_FAILURES = "consecutive_failures"
CONF_TOTAL_FAILURES = "total_failures"
CONF_RECOVERY_ATTEMPTS = "recovery_attempts"
CONF_SUCCESSFUL_RECOVERIES = "successful_recoveries"
CONF_FAILED_RECOVERIES = "failed_recoveries"


def temperature_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def diagnostic_counter_schema():
    return sensor.sensor_schema(
        icon=ICON_COUNTER,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_LEAFSENSE_AMG8833_ID): cv.use_id(
            LeafSenseAmg8833Component
        ),
        cv.Optional(CONF_MINIMUM_TEMPERATURE): temperature_schema(),
        cv.Optional(CONF_MAXIMUM_TEMPERATURE): temperature_schema(),
        cv.Optional(CONF_AVERAGE_TEMPERATURE): temperature_schema(),
        cv.Optional(CONF_THERMISTOR_TEMPERATURE): temperature_schema(),
        cv.Optional(CONF_FRAME_COUNT): diagnostic_counter_schema(),
        cv.Optional(CONF_VALID_PIXEL_COUNT): diagnostic_counter_schema(),
        cv.Optional(
            CONF_ACTIVE_INTERRUPT_PIXEL_COUNT
        ): diagnostic_counter_schema(),
        cv.Optional(CONF_CONSECUTIVE_FAILURES): diagnostic_counter_schema(),
        cv.Optional(CONF_TOTAL_FAILURES): diagnostic_counter_schema(),
        cv.Optional(CONF_RECOVERY_ATTEMPTS): diagnostic_counter_schema(),
        cv.Optional(CONF_SUCCESSFUL_RECOVERIES): diagnostic_counter_schema(),
        cv.Optional(CONF_FAILED_RECOVERIES): diagnostic_counter_schema(),
    }
)


async def register_optional_sensor(config, parent, key, setter):
    if key not in config:
        return

    sens = await sensor.new_sensor(config[key])
    cg.add(getattr(parent, setter)(sens))


async def to_code(config):
    parent = await cg.get_variable(
        config[CONF_LEAFSENSE_AMG8833_ID]
    )

    definitions = (
        (
            CONF_MINIMUM_TEMPERATURE,
            "set_minimum_temperature_sensor",
        ),
        (
            CONF_MAXIMUM_TEMPERATURE,
            "set_maximum_temperature_sensor",
        ),
        (
            CONF_AVERAGE_TEMPERATURE,
            "set_average_temperature_sensor",
        ),
        (
            CONF_THERMISTOR_TEMPERATURE,
            "set_thermistor_temperature_sensor",
        ),
        (CONF_FRAME_COUNT, "set_frame_count_sensor"),
        (
            CONF_VALID_PIXEL_COUNT,
            "set_valid_pixel_count_sensor",
        ),
        (
            CONF_ACTIVE_INTERRUPT_PIXEL_COUNT,
            "set_active_interrupt_pixel_count_sensor",
        ),
        (
            CONF_CONSECUTIVE_FAILURES,
            "set_consecutive_failures_sensor",
        ),
        (
            CONF_TOTAL_FAILURES,
            "set_total_failures_sensor",
        ),
        (
            CONF_RECOVERY_ATTEMPTS,
            "set_recovery_attempts_sensor",
        ),
        (
            CONF_SUCCESSFUL_RECOVERIES,
            "set_successful_recoveries_sensor",
        ),
        (
            CONF_FAILED_RECOVERIES,
            "set_failed_recoveries_sensor",
        ),
    )

    for key, setter in definitions:
        await register_optional_sensor(
            config,
            parent,
            key,
            setter,
        )
