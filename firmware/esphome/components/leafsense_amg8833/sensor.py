from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
)

from . import (
    LeafSenseAmg8833Component,
)


CONF_LEAFSENSE_AMG8833_ID = (
    "leafsense_amg8833_id"
)

CONF_FRAME_COUNT = "frame_count"

CONF_CONSECUTIVE_FAILURES = (
    "consecutive_failures"
)

CONF_TOTAL_FAILURES = "total_failures"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(
            CONF_LEAFSENSE_AMG8833_ID
        ): cv.use_id(
            LeafSenseAmg8833Component
        ),
        cv.Optional(
            CONF_FRAME_COUNT
        ): sensor.sensor_schema(
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=(
                ENTITY_CATEGORY_DIAGNOSTIC
            ),
        ),
        cv.Optional(
            CONF_CONSECUTIVE_FAILURES
        ): sensor.sensor_schema(
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=(
                ENTITY_CATEGORY_DIAGNOSTIC
            ),
        ),
        cv.Optional(
            CONF_TOTAL_FAILURES
        ): sensor.sensor_schema(
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
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

    if CONF_FRAME_COUNT in config:
        sens = await sensor.new_sensor(
            config[CONF_FRAME_COUNT]
        )

        cg.add(
            parent.set_frame_count_sensor(
                sens
            )
        )

    if CONF_CONSECUTIVE_FAILURES in config:
        sens = await sensor.new_sensor(
            config[
                CONF_CONSECUTIVE_FAILURES
            ]
        )

        cg.add(
            parent.set_consecutive_failures_sensor(
                sens
            )
        )

    if CONF_TOTAL_FAILURES in config:
        sens = await sensor.new_sensor(
            config[
                CONF_TOTAL_FAILURES
            ]
        )

        cg.add(
            parent.set_total_failures_sensor(
                sens
            )
        )