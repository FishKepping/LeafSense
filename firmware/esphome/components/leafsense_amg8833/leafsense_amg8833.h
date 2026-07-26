#pragma once

#include <cstdint>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace leafsense_amg8833 {

/**
 * @brief ESPHome integration scaffold for LeafSense AMG8833.
 *
 * Milestone 1.8 verifies the ESPHome component lifecycle, I2C
 * communication, configuration, and diagnostic entities.
 *
 * Thermal-frame acquisition through the platform-independent
 * LeafSense driver is added in milestone 1.9.
 */
class LeafSenseAmg8833Component
    : public PollingComponent,
      public i2c::I2CDevice
{
public:
    void setup() override;

    void update() override;

    void dump_config() override;

    float get_setup_priority() const override;

    void set_include_interrupt_map(
        bool include_interrupt_map);

    void set_recovery_enabled(
        bool recovery_enabled);

    void set_recovery_failure_threshold(
        std::uint8_t failure_threshold);

    void set_moving_average_enabled(
        bool moving_average_enabled);

    void set_frame_count_sensor(
        sensor::Sensor* frame_count_sensor);

    void set_consecutive_failures_sensor(
        sensor::Sensor*
            consecutive_failures_sensor);

    void set_total_failures_sensor(
        sensor::Sensor*
            total_failures_sensor);

    void set_connected_binary_sensor(
        binary_sensor::BinarySensor*
            connected_binary_sensor);

    void set_driver_problem_binary_sensor(
        binary_sensor::BinarySensor*
            driver_problem_binary_sensor);

protected:
    static constexpr std::uint8_t
        POWER_CONTROL_REGISTER = 0x00;

    static constexpr std::uint8_t
        NORMAL_MODE_VALUE = 0x00;

    bool probe_sensor_();

    void publish_diagnostics_();

    bool include_interrupt_map_ = false;

    bool recovery_enabled_ = true;

    bool moving_average_enabled_ = false;

    std::uint8_t recovery_failure_threshold_ = 3;

    bool connected_ = false;

    std::uint32_t frame_count_ = 0;

    std::uint32_t consecutive_failures_ = 0;

    std::uint32_t total_failures_ = 0;

    sensor::Sensor* frame_count_sensor_ = nullptr;

    sensor::Sensor*
        consecutive_failures_sensor_ = nullptr;

    sensor::Sensor* total_failures_sensor_ = nullptr;

    binary_sensor::BinarySensor*
        connected_binary_sensor_ = nullptr;

    binary_sensor::BinarySensor*
        driver_problem_binary_sensor_ = nullptr;
};

}  // namespace leafsense_amg8833
}  // namespace esphome