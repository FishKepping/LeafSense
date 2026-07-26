from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
)

CODEOWNERS = ["@FishKepping"]

DEPENDENCIES = ["i2c"]

MULTI_CONF = True

AUTO_LOAD = [
    "binary_sensor",
    "sensor",
]

CONF_INCLUDE_INTERRUPT_MAP = "include_interrupt_map"

CONF_RECOVERY_ENABLED = "recovery_enabled"

CONF_RECOVERY_FAILURE_THRESHOLD = "recovery_failure_threshold"

CONF_MOVING_AVERAGE_ENABLED = "moving_average_enabled"


leafsense_amg8833_ns = cg.esphome_ns.namespace(
    "leafsense_amg8833"
)

LeafSenseAmg8833Component = (
    leafsense_amg8833_ns.class_(
        "LeafSenseAmg8833Component",
        cg.PollingComponent,
        i2c.I2CDevice,
    )
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(
                LeafSenseAmg8833Component
            ),
            cv.Optional(
                CONF_INCLUDE_INTERRUPT_MAP,
                default=False,
            ): cv.boolean,
            cv.Optional(
                CONF_RECOVERY_ENABLED,
                default=True,
            ): cv.boolean,
            cv.Optional(
                CONF_RECOVERY_FAILURE_THRESHOLD,
                default=3,
            ): cv.int_range(
                min=1,
                max=255,
            ),
            cv.Optional(
                CONF_MOVING_AVERAGE_ENABLED,
                default=False,
            ): cv.boolean,
        }
    )
    .extend(
        cv.polling_component_schema(
            "10s"
        )
    )
    .extend(
        i2c.i2c_device_schema(
            default_address=0x69
        )
    )
)


FINAL_VALIDATE_SCHEMA = (
    i2c.final_validate_device_schema(
        "leafsense_amg8833",
        min_frequency="10kHz",
        max_frequency="400kHz",
    )
)


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID]
    )

    await cg.register_component(
        var,
        config,
    )

    await i2c.register_i2c_device(
        var,
        config,
    )

    cg.add(
        var.set_include_interrupt_map(
            config[
                CONF_INCLUDE_INTERRUPT_MAP
            ]
        )
    )

    cg.add(
        var.set_recovery_enabled(
            config[
                CONF_RECOVERY_ENABLED
            ]
        )
    )

    cg.add(
        var.set_recovery_failure_threshold(
            config[
                CONF_RECOVERY_FAILURE_THRESHOLD
            ]
        )
    )

    cg.add(
        var.set_moving_average_enabled(
            config[
                CONF_MOVING_AVERAGE_ENABLED
            ]
        )
    )