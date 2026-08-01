#pragma once

#include <cstddef>
#include <cstdint>

#include "amg8833_driver.h"
#include "thermal_frame.h"

namespace leafsense::drivers {

/**
 * @brief Statistics calculated from a valid thermal frame.
 */
struct Amg8833FrameSummary
{
    float minimum_temperature = 0.0f;

    float maximum_temperature = 0.0f;

    float average_temperature = 0.0f;

    float thermistor_temperature = 0.0f;

    std::size_t valid_pixel_count = 0;

    /**
     * True when the statistics were calculated from a valid frame
     * containing at least one valid pixel.
     */
    bool available = false;
};

/**
 * @brief Complete sensor-facing result of one capture operation.
 *
 * A snapshot can contain a successful frame while still being
 * incomplete if an optional interrupt-map read fails.
 */
struct Amg8833Snapshot
{
    ThermalFrame frame;

    Amg8833FrameSummary summary;

    Amg8833Status status;

    Amg8833InterruptMap interrupt_map;

    Amg8833DriverHealth health;

    Amg8833DriverError frame_error =
        Amg8833DriverError::None;

    Amg8833DriverError interrupt_map_error =
        Amg8833DriverError::None;

    bool interrupt_map_requested = false;

    bool interrupt_map_available = false;

    bool frame_recovery_attempted = false;

    bool frame_recovery_succeeded = false;

    bool interrupt_map_recovery_attempted = false;

    bool interrupt_map_recovery_succeeded = false;

    /**
     * Return true when frame acquisition completed successfully.
     *
     * A frame can be acquired successfully but still be marked invalid
     * because the sensor reported an overflow.
     */
    bool frameReadSucceeded() const;

    /**
     * Return true when a usable thermal frame is available.
     */
    bool frameAvailable() const;

    /**
     * Return true when all requested operations completed.
     *
     * When no interrupt map was requested, this is equivalent to
     * frameReadSucceeded().
     */
    bool complete() const;

    /**
     * Return true when any automatic recovery was attempted.
     */
    bool recoveryAttempted() const;

    /**
     * Return true when every attempted recovery succeeded.
     *
     * Returns false when no recovery was attempted.
     */
    bool recoverySucceeded() const;
};

/**
 * @brief Produces complete sensor-facing AMG8833 snapshots.
 *
 * This class coordinates frame acquisition, optional interrupt-map
 * acquisition, frame statistics, and driver-health reporting.
 *
 * It owns no sensor state. All hardware access remains inside
 * Amg8833Driver.
 *
 * The implementation performs no heap allocation.
 */
class Amg8833SnapshotReader
{
public:
    explicit Amg8833SnapshotReader(
        Amg8833Driver& driver);

    /**
     * Capture a processed thermal frame and optionally read the
     * hardware interrupt map.
     *
     * The interrupt map is read only when:
     *
     *     include_interrupt_map is true
     *     and frame acquisition succeeded
     *
     * This prevents a second bus operation after a failed frame read.
     */
    Amg8833Snapshot capture(
        std::uint32_t timestamp_ms,
        bool include_interrupt_map = false);

private:
    static Amg8833FrameSummary summarize(
        const ThermalFrame& frame);

    Amg8833Driver& driver_;
};

}  // namespace leafsense::drivers