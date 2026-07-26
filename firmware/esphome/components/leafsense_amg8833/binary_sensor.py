from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import (
    LeafSenseAmg8833Component,
)


CONF_LEAFSENSE_AMG8833_ID = (
    "leafsense_amg8833_id"
)

CONF_CONNECTED = "connected"

CONF_DRIVER_HEALTHY = "driver_healthy"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(
            CONF_LEAFSENSE_AMG8833_ID
        ): cv.use_id(
            LeafSenseAmg8833Component
        ),
        cv.Optional(
            CONF_CONNECTED
        ): binary_sensor.binary_sensor_schema(
            device_class=(
                DEVICE_CLASS_CONNECTIVITY
            ),
            entity_category=(
                ENTITY_CATEGORY_DIAGNOSTIC
            ),
        ),
        cv.Optional(
            CONF_DRIVER_HEALTHY
        ): binary_sensor.binary_sensor_schema(
            device_class=(
                DEVICE_CLASS_PROBLEM
            ),
            entity_category=(
                ENTITY_CATEGORY_DIAGNOSTIC
            ),
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(
        config[
            CONF_LEAFSENSE_AMG8833_ID
        ]
    )

    if CONF_CONNECTED in config:
        sens = (
            await binary_sensor
            .new_binary_sensor(
                config[CONF_CONNECTED]
            )
        )

        cg.add(
            parent.set_connected_binary_sensor(
                sens
            )
        )

    if CONF_DRIVER_HEALTHY in config:
        sens = (
            await binary_sensor
            .new_binary_sensor(
                config[CONF_DRIVER_HEALTHY]
            )
        )

        cg.add(
            parent.set_driver_problem_binary_sensor(
                sens
            )
        )