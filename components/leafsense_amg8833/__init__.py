from __future__ import annotations

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["api", "binary_sensor", "sensor", "text_sensor"]

leafsense_ns = cg.esphome_ns.namespace("leafsense_amg8833")
LeafSenseAmg8833Component = leafsense_ns.class_(
    "LeafSenseAmg8833Component",
    cg.PollingComponent,
    i2c.I2CDevice,
)

CONF_INCLUDE_INTERRUPT_MAP = "include_interrupt_map"
CONF_RECOVERY_ENABLED = "recovery_enabled"
CONF_RECOVERY_FAILURE_THRESHOLD = "recovery_failure_threshold"
CONF_MOVING_AVERAGE_ENABLED = "moving_average_enabled"
CONF_DEAD_PIXEL_CORRECTION_ENABLED = "dead_pixel_correction_enabled"
CONF_TEMPORAL_SMOOTHING_ENABLED = "temporal_smoothing_enabled"
CONF_TEMPORAL_SMOOTHING_ALPHA = "temporal_smoothing_alpha"
CONF_SPATIAL_MEDIAN_ENABLED = "spatial_median_enabled"
CONF_RECTANGLE_ROI = "rectangle_roi"
CONF_X = "x"
CONF_Y = "y"
CONF_WIDTH = "width"
CONF_HEIGHT = "height"

RECTANGLE_ROI_SCHEMA = cv.Schema({
    cv.Required(CONF_X): cv.int_range(min=0, max=7),
    cv.Required(CONF_Y): cv.int_range(min=0, max=7),
    cv.Required(CONF_WIDTH): cv.int_range(min=1, max=8),
    cv.Required(CONF_HEIGHT): cv.int_range(min=1, max=8),
})

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(LeafSenseAmg8833Component),
        cv.Optional(CONF_INCLUDE_INTERRUPT_MAP, default=True): cv.boolean,
        cv.Optional(CONF_RECOVERY_ENABLED, default=True): cv.boolean,
        cv.Optional(CONF_RECOVERY_FAILURE_THRESHOLD, default=3): cv.int_range(min=1, max=255),
        cv.Optional(CONF_MOVING_AVERAGE_ENABLED, default=False): cv.boolean,
        cv.Optional(CONF_DEAD_PIXEL_CORRECTION_ENABLED, default=True): cv.boolean,
        cv.Optional(CONF_TEMPORAL_SMOOTHING_ENABLED, default=True): cv.boolean,
        cv.Optional(CONF_TEMPORAL_SMOOTHING_ALPHA, default=0.35): cv.float_range(min=0.0, max=1.0),
        cv.Optional(CONF_SPATIAL_MEDIAN_ENABLED, default=False): cv.boolean,
        cv.Optional(CONF_RECTANGLE_ROI): RECTANGLE_ROI_SCHEMA,
    })
    .extend(cv.polling_component_schema("2s"))
    .extend(i2c.i2c_device_schema(default_address=0x69))
)

FINAL_VALIDATE_SCHEMA = i2c.final_validate_device_schema(
    "leafsense_amg8833", min_frequency="10kHz", max_frequency="400kHz"
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add_build_flag("-Isrc/esphome/components/leafsense_amg8833")
    cg.add(var.set_include_interrupt_map(config[CONF_INCLUDE_INTERRUPT_MAP]))
    cg.add(var.set_recovery_enabled(config[CONF_RECOVERY_ENABLED]))
    cg.add(var.set_recovery_failure_threshold(config[CONF_RECOVERY_FAILURE_THRESHOLD]))
    cg.add(var.set_moving_average_enabled(config[CONF_MOVING_AVERAGE_ENABLED]))
    cg.add(var.set_dead_pixel_correction_enabled(config[CONF_DEAD_PIXEL_CORRECTION_ENABLED]))
    cg.add(var.set_temporal_smoothing_enabled(config[CONF_TEMPORAL_SMOOTHING_ENABLED]))
    cg.add(var.set_temporal_smoothing_alpha(config[CONF_TEMPORAL_SMOOTHING_ALPHA]))
    cg.add(var.set_spatial_median_enabled(config[CONF_SPATIAL_MEDIAN_ENABLED]))

    if CONF_RECTANGLE_ROI in config:
        roi = config[CONF_RECTANGLE_ROI]
        cg.add(var.set_rectangle_roi(roi[CONF_X], roi[CONF_Y], roi[CONF_WIDTH], roi[CONF_HEIGHT]))
