from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import LeafSenseAmg8833Component

CONF_LEAFSENSE_AMG8833_ID = "leafsense_amg8833_id"
CONF_THERMAL_FRAME = "thermal_frame"

CONFIG_SCHEMA = cv.Schema({
    cv.Required(CONF_LEAFSENSE_AMG8833_ID): cv.use_id(LeafSenseAmg8833Component),
    cv.Optional(CONF_THERMAL_FRAME): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEAFSENSE_AMG8833_ID])
    if CONF_THERMAL_FRAME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_THERMAL_FRAME])
        cg.add(parent.set_thermal_frame_text_sensor(sens))
