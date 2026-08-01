#include "amg8833_telemetry.h"

namespace leafsense::drivers {

bool Amg8833Telemetry::temperaturesAvailable() const
{
    return temperature_values_available &&
           frame_available;
}

bool Amg8833Telemetry::fullyOperational() const
{
    return snapshot_complete &&
           frame_available &&
           driver_initialized &&
           driver_healthy;
}

bool Amg8833Telemetry::overflowDetected() const
{
    return pixel_temperature_overflow ||
           thermistor_overflow;
}

bool Amg8833Telemetry::interruptDetected() const
{
    return sensor_interrupt_active ||
           any_interrupt_pixel_active;
}

Amg8833Telemetry
Amg8833TelemetryProjector::project(
    const Amg8833Snapshot& snapshot)
{
    Amg8833Telemetry telemetry;

    telemetry.frame_read_succeeded =
        snapshot.frameReadSucceeded();

    telemetry.frame_available =
        snapshot.frameAvailable();

    telemetry.snapshot_complete =
        snapshot.complete();

    telemetry.temperature_values_available =
        snapshot.summary.available;

    telemetry.interrupt_map_requested =
        snapshot.interrupt_map_requested;

    telemetry.interrupt_map_available =
        snapshot.interrupt_map_available;

    telemetry.frame_error =
        snapshot.frame_error;

    telemetry.interrupt_map_error =
        snapshot.interrupt_map_error;

    telemetry.frame_number =
        snapshot.frame.frameNumber();

    telemetry.timestamp_ms =
        snapshot.frame.timestampMs();

    telemetry.valid_pixel_count =
        snapshot.summary.valid_pixel_count;

    if (snapshot.summary.available)
    {
        telemetry.minimum_temperature =
            snapshot.summary.minimum_temperature;

        telemetry.maximum_temperature =
            snapshot.summary.maximum_temperature;

        telemetry.average_temperature =
            snapshot.summary.average_temperature;

        telemetry.thermistor_temperature =
            snapshot.summary.thermistor_temperature;
    }

    telemetry.sensor_interrupt_active =
        snapshot.status.interrupt;

    telemetry.pixel_temperature_overflow =
        snapshot.status.pixelTemperatureOverflow;

    telemetry.thermistor_overflow =
        snapshot.status.thermistorOverflow;

    if (snapshot.interrupt_map_available)
    {
        telemetry.any_interrupt_pixel_active =
            snapshot.interrupt_map.any();

        telemetry.active_interrupt_pixel_count =
            snapshot.interrupt_map.activeCount();
    }

    telemetry.driver_initialized =
        snapshot.health.initialized;

    telemetry.driver_healthy =
        snapshot.health.healthy();

    telemetry.consecutive_failures =
        snapshot.health.consecutive_failures;

    telemetry.total_failures =
        snapshot.health.total_failures;

    telemetry.recovery_attempts =
        snapshot.health.recovery_attempts;

    telemetry.successful_recoveries =
        snapshot.health.successful_recoveries;

    telemetry.failed_recoveries =
        snapshot.health.failed_recoveries;

    telemetry.recovery_attempted =
        snapshot.recoveryAttempted();

    telemetry.recovery_succeeded =
        snapshot.recoverySucceeded();

    return telemetry;
}

}  // namespace leafsense::drivers