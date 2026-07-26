#pragma once

#include <cstdint>
#include <memory>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "esphome_bus.h"
#include "publisher.h"
#include "rectangle_roi_processor.h"

#include "leafsense/drivers/amg8833_driver.h"
#include "leafsense/drivers/amg8833_snapshot.h"
#include "leafsense/drivers/amg8833_telemetry.h"

namespace esphome {
namespace leafsense_amg8833 {

class LeafSenseAmg8833Component
    : public PollingComponent,
      public i2c::I2CDevice
{
public:
    LeafSenseAmg8833Component();

    void setup() override;
    void update() override;
    void dump_config() override;
    float get_setup_priority() const override;

    void set_include_interrupt_map(bool value);
    void set_recovery_enabled(bool value);
    void set_recovery_failure_threshold(std::uint8_t value);
    void set_moving_average_enabled(bool value);

    void set_rectangle_roi(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t width,
        std::uint8_t height);

    void set_minimum_temperature_sensor(sensor::Sensor* value);
    void set_maximum_temperature_sensor(sensor::Sensor* value);
    void set_average_temperature_sensor(sensor::Sensor* value);
    void set_thermistor_temperature_sensor(sensor::Sensor* value);

    void set_roi_minimum_temperature_sensor(sensor::Sensor* value);
    void set_roi_maximum_temperature_sensor(sensor::Sensor* value);
    void set_roi_average_temperature_sensor(sensor::Sensor* value);
    void set_roi_pixel_count_sensor(sensor::Sensor* value);

    void set_frame_count_sensor(sensor::Sensor* value);
    void set_valid_pixel_count_sensor(sensor::Sensor* value);
    void set_active_interrupt_pixel_count_sensor(sensor::Sensor* value);
    void set_consecutive_failures_sensor(sensor::Sensor* value);
    void set_total_failures_sensor(sensor::Sensor* value);
    void set_recovery_attempts_sensor(sensor::Sensor* value);
    void set_successful_recoveries_sensor(sensor::Sensor* value);
    void set_failed_recoveries_sensor(sensor::Sensor* value);

    void set_connected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_driver_problem_binary_sensor(binary_sensor::BinarySensor* value);
    void set_frame_available_binary_sensor(binary_sensor::BinarySensor* value);
    void set_overflow_detected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_interrupt_detected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_recovery_active_binary_sensor(binary_sensor::BinarySensor* value);
    void set_roi_available_binary_sensor(binary_sensor::BinarySensor* value);

protected:
    bool initializeDriver_();
    static const char* driverErrorName_(
        leafsense::drivers::Amg8833DriverError error);

    bool include_interrupt_map_ = false;
    leafsense::drivers::Amg8833DriverConfig driver_config_;

    EspHomeAmg8833Bus bus_adapter_;
    std::unique_ptr<leafsense::drivers::Amg8833Driver> driver_;
    std::unique_ptr<leafsense::drivers::Amg8833SnapshotReader> snapshot_reader_;

    RectangleRoiProcessor roi_processor_;
    Amg8833TelemetryPublisher publisher_;
};

}  // namespace leafsense_amg8833
}  // namespace esphome
