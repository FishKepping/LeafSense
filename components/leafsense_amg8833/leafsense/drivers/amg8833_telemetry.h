#pragma once

#include <cstddef>
#include <cstdint>

#include "leafsense/drivers/amg8833_driver.h"
#include "leafsense/drivers/amg8833_snapshot.h"

namespace leafsense::drivers {

/**
 * @brief Flat, publication-ready representation of an AMG8833 snapshot.
 *
 * This structure deliberately contains only scalar values, flags, counters,
 * and driver-error enums. Platform integrations such as ESPHome can publish
 * these fields without understanding ThermalFrame, FrameStatistics, or the
 * snapshot acquisition workflow.
 *
 * Numeric temperature values are meaningful only when
 * temperature_values_available is true.
 */
struct Amg8833Telemetry
{
    /*
     * Overall availability.
     */
    bool frame_read_succeeded = false;

    bool frame_available = false;

    bool snapshot_complete = false;

    bool temperature_values_available = false;

    bool interrupt_map_requested = false;

    bool interrupt_map_available = false;

    /*
     * Temperature measurements.
     */
    float minimum_temperature = 0.0f;

    float maximum_temperature = 0.0f;

    float average_temperature = 0.0f;

    float thermistor_temperature = 0.0f;

    /*
     * Frame metadata.
     */
    std::uint32_t frame_number = 0;

    std::uint32_t timestamp_ms = 0;

    std::size_t valid_pixel_count = 0;

    /*
     * Sensor status.
     */
    bool sensor_interrupt_active = false;

    bool pixel_temperature_overflow = false;

    bool thermistor_overflow = false;

    /*
     * Interrupt-map summary.
     */
    bool any_interrupt_pixel_active = false;

    std::size_t active_interrupt_pixel_count = 0;

    /*
     * Driver health.
     */
    bool driver_initialized = false;

    bool driver_healthy = false;

    std::uint32_t consecutive_failures = 0;

    std::uint32_t total_failures = 0;

    std::uint32_t recovery_attempts = 0;

    std::uint32_t successful_recoveries = 0;

    std::uint32_t failed_recoveries = 0;

    /*
     * Recovery state for this snapshot.
     */
    bool recovery_attempted = false;

    bool recovery_succeeded = false;

    /*
     * Detailed operation errors.
     */
    Amg8833DriverError frame_error =
        Amg8833DriverError::None;

    Amg8833DriverError interrupt_map_error =
        Amg8833DriverError::None;

    /**
     * Return true when the telemetry contains usable thermal measurements.
     */
    bool temperaturesAvailable() const;

    /**
     * Return true when the snapshot completed and the driver is healthy.
     *
     * This is stricter than frame_available. A frame may remain usable when
     * an optional interrupt-map read fails.
     */
    bool fullyOperational() const;

    /**
     * Return true when the sensor reported either overflow condition.
     */
    bool overflowDetected() const;

    /**
     * Return true when any interrupt indication is active.
     *
     * This includes the status-register interrupt flag and any decoded
     * interrupt-map pixels.
     */
    bool interruptDetected() const;
};

/**
 * @brief Converts rich AMG8833 snapshots into flat telemetry.
 *
 * This class contains no state and performs no hardware access.
 */
class Amg8833TelemetryProjector
{
public:
    /**
     * Convert a snapshot into publication-ready telemetry.
     */
    static Amg8833Telemetry project(
        const Amg8833Snapshot& snapshot);
};

}  // namespace leafsense::drivers