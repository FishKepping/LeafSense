#include "leafsense_amg8833.h"

#include "esphome/core/log.h"

namespace esphome {
namespace leafsense_amg8833 {

static const char *const TAG =
    "leafsense_amg8833";

void LeafSenseAmg8833Component::setup()
{
    ESP_LOGCONFIG(
        TAG,
        "Setting up LeafSense AMG8833..."
    );

    connected_ =
        probe_sensor_();

    if (!connected_)
    {
        ++consecutive_failures_;
        ++total_failures_;

        mark_failed();

        ESP_LOGE(
            TAG,
            "AMG8833 did not acknowledge at "
            "I2C address 0x%02X",
            address_
        );
    }
    else
    {
        consecutive_failures_ = 0;

        ESP_LOGCONFIG(
            TAG,
            "AMG8833 communication established"
        );
    }

    publish_diagnostics_();
}

void LeafSenseAmg8833Component::update()
{
    if (is_failed())
    {
        publish_diagnostics_();

        return;
    }

    const bool probe_succeeded =
        probe_sensor_();

    if (!probe_succeeded)
    {
        connected_ = false;

        ++consecutive_failures_;
        ++total_failures_;

        status_set_warning();

        ESP_LOGW(
            TAG,
            "AMG8833 communication check failed"
        );

        publish_diagnostics_();

        return;
    }

    connected_ = true;
    consecutive_failures_ = 0;

    ++frame_count_;

    status_clear_warning();

    publish_diagnostics_();
}

void LeafSenseAmg8833Component::dump_config()
{
    ESP_LOGCONFIG(
        TAG,
        "LeafSense AMG8833:"
    );

    LOG_I2C_DEVICE(this);
    LOG_UPDATE_INTERVAL(this);

    ESP_LOGCONFIG(
        TAG,
        "  Include interrupt map: %s",
        include_interrupt_map_
            ? "true"
            : "false"
    );

    ESP_LOGCONFIG(
        TAG,
        "  Moving average enabled: %s",
        moving_average_enabled_
            ? "true"
            : "false"
    );

    ESP_LOGCONFIG(
        TAG,
        "  Automatic recovery enabled: %s",
        recovery_enabled_
            ? "true"
            : "false"
    );

    ESP_LOGCONFIG(
        TAG,
        "  Recovery failure threshold: %u",
        static_cast<unsigned>(
            recovery_failure_threshold_
        )
    );

    LOG_SENSOR(
        "  ",
        "Frame Count",
        frame_count_sensor_
    );

    LOG_SENSOR(
        "  ",
        "Consecutive Failures",
        consecutive_failures_sensor_
    );

    LOG_SENSOR(
        "  ",
        "Total Failures",
        total_failures_sensor_
    );

    LOG_BINARY_SENSOR(
        "  ",
        "Connected",
        connected_binary_sensor_
    );

    LOG_BINARY_SENSOR(
        "  ",
        "Driver Problem",
        driver_problem_binary_sensor_
    );

    if (is_failed())
    {
        ESP_LOGE(
            TAG,
            "Communication setup failed"
        );
    }
}

float
LeafSenseAmg8833Component::get_setup_priority() const
{
    return setup_priority::DATA;
}

void LeafSenseAmg8833Component::
    set_include_interrupt_map(
        bool include_interrupt_map)
{
    include_interrupt_map_ =
        include_interrupt_map;
}

void LeafSenseAmg8833Component::
    set_recovery_enabled(
        bool recovery_enabled)
{
    recovery_enabled_ =
        recovery_enabled;
}

void LeafSenseAmg8833Component::
    set_recovery_failure_threshold(
        std::uint8_t failure_threshold)
{
    if (failure_threshold == 0U)
    {
        recovery_failure_threshold_ = 1U;

        return;
    }

    recovery_failure_threshold_ =
        failure_threshold;
}

void LeafSenseAmg8833Component::
    set_moving_average_enabled(
        bool moving_average_enabled)
{
    moving_average_enabled_ =
        moving_average_enabled;
}

void LeafSenseAmg8833Component::
    set_frame_count_sensor(
        sensor::Sensor *frame_count_sensor)
{
    frame_count_sensor_ =
        frame_count_sensor;
}

void LeafSenseAmg8833Component::
    set_consecutive_failures_sensor(
        sensor::Sensor *
            consecutive_failures_sensor)
{
    consecutive_failures_sensor_ =
        consecutive_failures_sensor;
}

void LeafSenseAmg8833Component::
    set_total_failures_sensor(
        sensor::Sensor *total_failures_sensor)
{
    total_failures_sensor_ =
        total_failures_sensor;
}

void LeafSenseAmg8833Component::
    set_connected_binary_sensor(
        binary_sensor::BinarySensor *
            connected_binary_sensor)
{
    connected_binary_sensor_ =
        connected_binary_sensor;
}

void LeafSenseAmg8833Component::
    set_driver_problem_binary_sensor(
        binary_sensor::BinarySensor *
            driver_problem_binary_sensor)
{
    driver_problem_binary_sensor_ =
        driver_problem_binary_sensor;
}

bool LeafSenseAmg8833Component::probe_sensor_()
{
    const std::uint8_t value =
        NORMAL_MODE_VALUE;

    const i2c::ErrorCode result =
        write_register(
            POWER_CONTROL_REGISTER,
            &value,
            1U
        );

    return result ==
           i2c::ERROR_OK;
}

void LeafSenseAmg8833Component::
    publish_diagnostics_()
{
    if (frame_count_sensor_ != nullptr)
    {
        frame_count_sensor_->publish_state(
            static_cast<float>(
                frame_count_
            )
        );
    }

    if (consecutive_failures_sensor_ != nullptr)
    {
        consecutive_failures_sensor_->
            publish_state(
                static_cast<float>(
                    consecutive_failures_
                )
            );
    }

    if (total_failures_sensor_ != nullptr)
    {
        total_failures_sensor_->
            publish_state(
                static_cast<float>(
                    total_failures_
                )
            );
    }

    if (connected_binary_sensor_ != nullptr)
    {
        connected_binary_sensor_->
            publish_state(
                connected_
            );
    }

    if (driver_problem_binary_sensor_ != nullptr)
    {
        driver_problem_binary_sensor_->
            publish_state(
                !connected_ ||
                consecutive_failures_ > 0U
            );
    }
}

}  // namespace leafsense_amg8833
}  // namespace esphome