#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "leafsense/drivers/amg8833_telemetry.h"

namespace esphome {
namespace leafsense_amg8833 {

class Amg8833TelemetryPublisher
{
public:
    void publish(
        const leafsense::drivers::Amg8833Telemetry& telemetry);

    void publishUnavailable();

    void setMinimumTemperatureSensor(sensor::Sensor* value);
    void setMaximumTemperatureSensor(sensor::Sensor* value);
    void setAverageTemperatureSensor(sensor::Sensor* value);
    void setThermistorTemperatureSensor(sensor::Sensor* value);

    void setFrameCountSensor(sensor::Sensor* value);
    void setValidPixelCountSensor(sensor::Sensor* value);
    void setActiveInterruptPixelCountSensor(sensor::Sensor* value);
    void setConsecutiveFailuresSensor(sensor::Sensor* value);
    void setTotalFailuresSensor(sensor::Sensor* value);
    void setRecoveryAttemptsSensor(sensor::Sensor* value);
    void setSuccessfulRecoveriesSensor(sensor::Sensor* value);
    void setFailedRecoveriesSensor(sensor::Sensor* value);

    void setConnectedBinarySensor(binary_sensor::BinarySensor* value);
    void setDriverProblemBinarySensor(binary_sensor::BinarySensor* value);
    void setFrameAvailableBinarySensor(binary_sensor::BinarySensor* value);
    void setOverflowDetectedBinarySensor(binary_sensor::BinarySensor* value);
    void setInterruptDetectedBinarySensor(binary_sensor::BinarySensor* value);
    void setRecoveryActiveBinarySensor(binary_sensor::BinarySensor* value);

private:
    static void publishSensor(
        sensor::Sensor* target,
        float value);

    static void publishCounter(
        sensor::Sensor* target,
        std::uint32_t value);

    static void publishBinarySensor(
        binary_sensor::BinarySensor* target,
        bool value);

    sensor::Sensor* minimum_temperature_sensor_ = nullptr;
    sensor::Sensor* maximum_temperature_sensor_ = nullptr;
    sensor::Sensor* average_temperature_sensor_ = nullptr;
    sensor::Sensor* thermistor_temperature_sensor_ = nullptr;

    sensor::Sensor* frame_count_sensor_ = nullptr;
    sensor::Sensor* valid_pixel_count_sensor_ = nullptr;
    sensor::Sensor* active_interrupt_pixel_count_sensor_ = nullptr;
    sensor::Sensor* consecutive_failures_sensor_ = nullptr;
    sensor::Sensor* total_failures_sensor_ = nullptr;
    sensor::Sensor* recovery_attempts_sensor_ = nullptr;
    sensor::Sensor* successful_recoveries_sensor_ = nullptr;
    sensor::Sensor* failed_recoveries_sensor_ = nullptr;

    binary_sensor::BinarySensor* connected_binary_sensor_ = nullptr;
    binary_sensor::BinarySensor* driver_problem_binary_sensor_ = nullptr;
    binary_sensor::BinarySensor* frame_available_binary_sensor_ = nullptr;
    binary_sensor::BinarySensor* overflow_detected_binary_sensor_ = nullptr;
    binary_sensor::BinarySensor* interrupt_detected_binary_sensor_ = nullptr;
    binary_sensor::BinarySensor* recovery_active_binary_sensor_ = nullptr;
};

}  // namespace leafsense_amg8833
}  // namespace esphome
