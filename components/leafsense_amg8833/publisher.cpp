#include "publisher.h"

#include <cmath>
#include <cstdint>

namespace esphome {
namespace leafsense_amg8833 {

void Amg8833TelemetryPublisher::publish(
    const leafsense::drivers::Amg8833Telemetry& telemetry,
    const RectangleRoiResult& roi_result)
{
    if (telemetry.temperaturesAvailable())
    {
        publishSensor(minimum_temperature_sensor_, telemetry.minimum_temperature);
        publishSensor(maximum_temperature_sensor_, telemetry.maximum_temperature);
        publishSensor(average_temperature_sensor_, telemetry.average_temperature);
        publishSensor(
            thermistor_temperature_sensor_,
            telemetry.thermistor_temperature);
    }
    else
    {
        publishSensor(minimum_temperature_sensor_, NAN);
        publishSensor(maximum_temperature_sensor_, NAN);
        publishSensor(average_temperature_sensor_, NAN);
        publishSensor(thermistor_temperature_sensor_, NAN);
    }

    if (roi_result.available)
    {
        publishSensor(
            roi_minimum_temperature_sensor_,
            roi_result.minimum_temperature);
        publishSensor(
            roi_maximum_temperature_sensor_,
            roi_result.maximum_temperature);
        publishSensor(
            roi_average_temperature_sensor_,
            roi_result.average_temperature);
        publishCounter(
            roi_pixel_count_sensor_,
            static_cast<std::uint32_t>(roi_result.pixel_count));
    }
    else
    {
        publishSensor(roi_minimum_temperature_sensor_, NAN);
        publishSensor(roi_maximum_temperature_sensor_, NAN);
        publishSensor(roi_average_temperature_sensor_, NAN);
        publishCounter(roi_pixel_count_sensor_, 0U);
    }

    publishCounter(frame_count_sensor_, telemetry.frame_number);
    publishCounter(
        valid_pixel_count_sensor_,
        static_cast<std::uint32_t>(telemetry.valid_pixel_count));
    publishCounter(
        active_interrupt_pixel_count_sensor_,
        static_cast<std::uint32_t>(
            telemetry.active_interrupt_pixel_count));
    publishCounter(
        consecutive_failures_sensor_,
        telemetry.consecutive_failures);
    publishCounter(total_failures_sensor_, telemetry.total_failures);
    publishCounter(recovery_attempts_sensor_, telemetry.recovery_attempts);
    publishCounter(
        successful_recoveries_sensor_,
        telemetry.successful_recoveries);
    publishCounter(failed_recoveries_sensor_, telemetry.failed_recoveries);

    publishBinarySensor(
        connected_binary_sensor_,
        telemetry.driver_initialized &&
            telemetry.frame_read_succeeded);
    publishBinarySensor(
        driver_problem_binary_sensor_,
        !telemetry.fullyOperational());
    publishBinarySensor(
        frame_available_binary_sensor_,
        telemetry.frame_available);
    publishBinarySensor(
        overflow_detected_binary_sensor_,
        telemetry.overflowDetected());
    publishBinarySensor(
        interrupt_detected_binary_sensor_,
        telemetry.interruptDetected());
    publishBinarySensor(
        recovery_active_binary_sensor_,
        telemetry.recovery_attempted);
    publishBinarySensor(
        roi_available_binary_sensor_,
        roi_result.available);
}

void Amg8833TelemetryPublisher::publishUnavailable()
{
    publishSensor(minimum_temperature_sensor_, NAN);
    publishSensor(maximum_temperature_sensor_, NAN);
    publishSensor(average_temperature_sensor_, NAN);
    publishSensor(thermistor_temperature_sensor_, NAN);

    publishSensor(roi_minimum_temperature_sensor_, NAN);
    publishSensor(roi_maximum_temperature_sensor_, NAN);
    publishSensor(roi_average_temperature_sensor_, NAN);
    publishCounter(roi_pixel_count_sensor_, 0U);

    publishBinarySensor(connected_binary_sensor_, false);
    publishBinarySensor(driver_problem_binary_sensor_, true);
    publishBinarySensor(frame_available_binary_sensor_, false);
    publishBinarySensor(overflow_detected_binary_sensor_, false);
    publishBinarySensor(interrupt_detected_binary_sensor_, false);
    publishBinarySensor(recovery_active_binary_sensor_, false);
    publishBinarySensor(roi_available_binary_sensor_, false);
}

void Amg8833TelemetryPublisher::setMinimumTemperatureSensor(
    sensor::Sensor* value)
{
    minimum_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setMaximumTemperatureSensor(
    sensor::Sensor* value)
{
    maximum_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setAverageTemperatureSensor(
    sensor::Sensor* value)
{
    average_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setThermistorTemperatureSensor(
    sensor::Sensor* value)
{
    thermistor_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRoiMinimumTemperatureSensor(
    sensor::Sensor* value)
{
    roi_minimum_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRoiMaximumTemperatureSensor(
    sensor::Sensor* value)
{
    roi_maximum_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRoiAverageTemperatureSensor(
    sensor::Sensor* value)
{
    roi_average_temperature_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRoiPixelCountSensor(
    sensor::Sensor* value)
{
    roi_pixel_count_sensor_ = value;
}

void Amg8833TelemetryPublisher::setFrameCountSensor(sensor::Sensor* value)
{
    frame_count_sensor_ = value;
}

void Amg8833TelemetryPublisher::setValidPixelCountSensor(sensor::Sensor* value)
{
    valid_pixel_count_sensor_ = value;
}

void Amg8833TelemetryPublisher::setActiveInterruptPixelCountSensor(
    sensor::Sensor* value)
{
    active_interrupt_pixel_count_sensor_ = value;
}

void Amg8833TelemetryPublisher::setConsecutiveFailuresSensor(
    sensor::Sensor* value)
{
    consecutive_failures_sensor_ = value;
}

void Amg8833TelemetryPublisher::setTotalFailuresSensor(sensor::Sensor* value)
{
    total_failures_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRecoveryAttemptsSensor(sensor::Sensor* value)
{
    recovery_attempts_sensor_ = value;
}

void Amg8833TelemetryPublisher::setSuccessfulRecoveriesSensor(
    sensor::Sensor* value)
{
    successful_recoveries_sensor_ = value;
}

void Amg8833TelemetryPublisher::setFailedRecoveriesSensor(sensor::Sensor* value)
{
    failed_recoveries_sensor_ = value;
}

void Amg8833TelemetryPublisher::setConnectedBinarySensor(
    binary_sensor::BinarySensor* value)
{
    connected_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setDriverProblemBinarySensor(
    binary_sensor::BinarySensor* value)
{
    driver_problem_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setFrameAvailableBinarySensor(
    binary_sensor::BinarySensor* value)
{
    frame_available_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setOverflowDetectedBinarySensor(
    binary_sensor::BinarySensor* value)
{
    overflow_detected_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setInterruptDetectedBinarySensor(
    binary_sensor::BinarySensor* value)
{
    interrupt_detected_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRecoveryActiveBinarySensor(
    binary_sensor::BinarySensor* value)
{
    recovery_active_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::setRoiAvailableBinarySensor(
    binary_sensor::BinarySensor* value)
{
    roi_available_binary_sensor_ = value;
}

void Amg8833TelemetryPublisher::publishSensor(
    sensor::Sensor* target,
    float value)
{
    if (target != nullptr)
    {
        target->publish_state(value);
    }
}

void Amg8833TelemetryPublisher::publishCounter(
    sensor::Sensor* target,
    std::uint32_t value)
{
    publishSensor(target, static_cast<float>(value));
}

void Amg8833TelemetryPublisher::publishBinarySensor(
    binary_sensor::BinarySensor* target,
    bool value)
{
    if (target != nullptr)
    {
        target->publish_state(value);
    }
}

}  // namespace leafsense_amg8833
}  // namespace esphome
