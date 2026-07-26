#include "leafsense_component.h"

#include <utility>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace leafsense_amg8833 {

static const char* const TAG = "leafsense_amg8833";

LeafSenseAmg8833Component::LeafSenseAmg8833Component()
    : bus_adapter_(*this)
{
}

void LeafSenseAmg8833Component::setup()
{
    ESP_LOGCONFIG(TAG, "Setting up LeafSense AMG8833...");

    if (!initializeDriver_())
    {
        status_set_error();
        publisher_.publishUnavailable();

        ESP_LOGE(
            TAG,
            "AMG8833 initialization failed: %s",
            driverErrorName_(
                driver_ != nullptr
                    ? driver_->lastError()
                    : leafsense::drivers::Amg8833DriverError::NotInitialized));
        return;
    }

    status_clear_error();
    ESP_LOGCONFIG(TAG, "LeafSense AMG8833 driver initialized");
}

void LeafSenseAmg8833Component::update()
{
    if (driver_ == nullptr ||
        snapshot_reader_ == nullptr ||
        !driver_->initialized())
    {
        if (!initializeDriver_())
        {
            status_set_error();
            publisher_.publishUnavailable();

            ESP_LOGW(
                TAG,
                "AMG8833 reinitialization failed: %s",
                driverErrorName_(
                    driver_ != nullptr
                        ? driver_->lastError()
                        : leafsense::drivers::Amg8833DriverError::NotInitialized));
            return;
        }
    }

    const leafsense::drivers::Amg8833Snapshot snapshot =
        snapshot_reader_->capture(
            millis(),
            include_interrupt_map_);

    const leafsense::drivers::Amg8833Telemetry telemetry =
        leafsense::drivers::Amg8833TelemetryProjector::project(
            snapshot);

    const RectangleRoiResult roi_result =
        roi_processor_.process(snapshot.frame);

    publisher_.publish(telemetry, roi_result);

    if (telemetry.frame_read_succeeded)
    {
        status_clear_warning();
        status_clear_error();
    }
    else
    {
        status_set_warning();

        ESP_LOGW(
            TAG,
            "AMG8833 capture failed: %s",
            driverErrorName_(telemetry.frame_error));
    }

    if (telemetry.recovery_attempted)
    {
        ESP_LOGI(
            TAG,
            "Automatic recovery attempted: %s",
            telemetry.recovery_succeeded
                ? "succeeded"
                : "failed");
    }

    if (telemetry.temperaturesAvailable())
    {
        ESP_LOGD(
            TAG,
            "Frame %lu: min %.2f C, max %.2f C, avg %.2f C, thermistor %.2f C",
            static_cast<unsigned long>(telemetry.frame_number),
            telemetry.minimum_temperature,
            telemetry.maximum_temperature,
            telemetry.average_temperature,
            telemetry.thermistor_temperature);
    }
}

void LeafSenseAmg8833Component::dump_config()
{
    ESP_LOGCONFIG(TAG, "LeafSense AMG8833:");
    LOG_I2C_DEVICE(this);
    LOG_UPDATE_INTERVAL(this);

    ESP_LOGCONFIG(
        TAG,
        "  Interrupt map: %s",
        include_interrupt_map_ ? "enabled" : "disabled");
    ESP_LOGCONFIG(
        TAG,
        "  Moving average: %s",
        driver_config_.moving_average_enabled ? "enabled" : "disabled");
    ESP_LOGCONFIG(
        TAG,
        "  Automatic recovery: %s",
        driver_config_.recovery.enabled ? "enabled" : "disabled");
    ESP_LOGCONFIG(
        TAG,
        "  Recovery failure threshold: %lu",
        static_cast<unsigned long>(
            driver_config_.recovery.failure_threshold));

    if (roi_processor_.configured())
    {
        const leafsense::roi::RectangleRoi& roi =
            roi_processor_.roi();

        ESP_LOGCONFIG(
            TAG,
            "  Rectangle ROI: x=%u, y=%u, width=%u, height=%u, pixels=%lu",
            static_cast<unsigned>(roi.x()),
            static_cast<unsigned>(roi.y()),
            static_cast<unsigned>(roi.width()),
            static_cast<unsigned>(roi.height()),
            static_cast<unsigned long>(roi.pixelCount()));
    }
    else
    {
        ESP_LOGCONFIG(TAG, "  Rectangle ROI: disabled");
    }

    if (driver_ != nullptr)
    {
        const leafsense::drivers::Amg8833DriverHealth health =
            driver_->health();

        ESP_LOGCONFIG(
            TAG,
            "  Driver initialized: %s",
            health.initialized ? "yes" : "no");
        ESP_LOGCONFIG(
            TAG,
            "  Total failures: %lu",
            static_cast<unsigned long>(health.total_failures));
        ESP_LOGCONFIG(
            TAG,
            "  Recovery attempts: %lu",
            static_cast<unsigned long>(health.recovery_attempts));
    }
}

float LeafSenseAmg8833Component::get_setup_priority() const
{
    return setup_priority::DATA;
}

void LeafSenseAmg8833Component::set_include_interrupt_map(bool value)
{
    include_interrupt_map_ = value;
}

void LeafSenseAmg8833Component::set_recovery_enabled(bool value)
{
    driver_config_.recovery.enabled = value;
}

void LeafSenseAmg8833Component::set_recovery_failure_threshold(
    std::uint8_t value)
{
    driver_config_.recovery.failure_threshold =
        value == 0U ? 1U : value;
}

void LeafSenseAmg8833Component::set_moving_average_enabled(bool value)
{
    driver_config_.moving_average_enabled = value;
}


void LeafSenseAmg8833Component::set_rectangle_roi(
    std::uint8_t x,
    std::uint8_t y,
    std::uint8_t width,
    std::uint8_t height)
{
    roi_processor_.configure(x, y, width, height);
}

void LeafSenseAmg8833Component::set_minimum_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setMinimumTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_maximum_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setMaximumTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_average_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setAverageTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_thermistor_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setThermistorTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_roi_minimum_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setRoiMinimumTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_roi_maximum_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setRoiMaximumTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_roi_average_temperature_sensor(
    sensor::Sensor* value)
{
    publisher_.setRoiAverageTemperatureSensor(value);
}

void LeafSenseAmg8833Component::set_roi_pixel_count_sensor(
    sensor::Sensor* value)
{
    publisher_.setRoiPixelCountSensor(value);
}

void LeafSenseAmg8833Component::set_frame_count_sensor(
    sensor::Sensor* value)
{
    publisher_.setFrameCountSensor(value);
}

void LeafSenseAmg8833Component::set_valid_pixel_count_sensor(
    sensor::Sensor* value)
{
    publisher_.setValidPixelCountSensor(value);
}

void LeafSenseAmg8833Component::set_active_interrupt_pixel_count_sensor(
    sensor::Sensor* value)
{
    publisher_.setActiveInterruptPixelCountSensor(value);
}

void LeafSenseAmg8833Component::set_consecutive_failures_sensor(
    sensor::Sensor* value)
{
    publisher_.setConsecutiveFailuresSensor(value);
}

void LeafSenseAmg8833Component::set_total_failures_sensor(
    sensor::Sensor* value)
{
    publisher_.setTotalFailuresSensor(value);
}

void LeafSenseAmg8833Component::set_recovery_attempts_sensor(
    sensor::Sensor* value)
{
    publisher_.setRecoveryAttemptsSensor(value);
}

void LeafSenseAmg8833Component::set_successful_recoveries_sensor(
    sensor::Sensor* value)
{
    publisher_.setSuccessfulRecoveriesSensor(value);
}

void LeafSenseAmg8833Component::set_failed_recoveries_sensor(
    sensor::Sensor* value)
{
    publisher_.setFailedRecoveriesSensor(value);
}

void LeafSenseAmg8833Component::set_connected_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setConnectedBinarySensor(value);
}

void LeafSenseAmg8833Component::set_driver_problem_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setDriverProblemBinarySensor(value);
}

void LeafSenseAmg8833Component::set_frame_available_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setFrameAvailableBinarySensor(value);
}

void LeafSenseAmg8833Component::set_overflow_detected_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setOverflowDetectedBinarySensor(value);
}

void LeafSenseAmg8833Component::set_interrupt_detected_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setInterruptDetectedBinarySensor(value);
}

void LeafSenseAmg8833Component::set_recovery_active_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setRecoveryActiveBinarySensor(value);
}


void LeafSenseAmg8833Component::set_roi_available_binary_sensor(
    binary_sensor::BinarySensor* value)
{
    publisher_.setRoiAvailableBinarySensor(value);
}

bool LeafSenseAmg8833Component::initializeDriver_()
{
    driver_ = std::make_unique<leafsense::drivers::Amg8833Driver>(
        bus_adapter_,
        driver_config_);

    snapshot_reader_ =
        std::make_unique<leafsense::drivers::Amg8833SnapshotReader>(
            *driver_);

    return driver_->initialize();
}

const char* LeafSenseAmg8833Component::driverErrorName_(
    leafsense::drivers::Amg8833DriverError error)
{
    using Error = leafsense::drivers::Amg8833DriverError;

    switch (error)
    {
        case Error::None:
            return "none";
        case Error::NotInitialized:
            return "not initialized";
        case Error::InvalidInterruptThresholds:
            return "invalid interrupt thresholds";
        case Error::PowerControlWriteFailed:
            return "power control write failed";
        case Error::InitialResetWriteFailed:
            return "initial reset write failed";
        case Error::FrameRateWriteFailed:
            return "frame-rate write failed";
        case Error::MovingAverageWriteFailed:
            return "moving-average write failed";
        case Error::InterruptThresholdWriteFailed:
            return "interrupt-threshold write failed";
        case Error::InterruptControlWriteFailed:
            return "interrupt-control write failed";
        case Error::StatusClearWriteFailed:
            return "status clear write failed";
        case Error::StatusReadFailed:
            return "status read failed";
        case Error::ThermistorReadFailed:
            return "thermistor read failed";
        case Error::PixelReadFailed:
            return "pixel read failed";
        case Error::InterruptTableReadFailed:
            return "interrupt-table read failed";
        default:
            return "unknown";
    }
}

}  // namespace leafsense_amg8833
}  // namespace esphome
