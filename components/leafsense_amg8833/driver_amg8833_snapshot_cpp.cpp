#include "amg8833_snapshot.h"

#include <cstddef>

#include "frame_statistics.h"

namespace leafsense::drivers {

bool Amg8833Snapshot::frameReadSucceeded() const
{
    return frame_error ==
           Amg8833DriverError::None;
}

bool Amg8833Snapshot::frameAvailable() const
{
    return frameReadSucceeded() &&
           frame.isValid() &&
           summary.available;
}

bool Amg8833Snapshot::complete() const
{
    if (!frameReadSucceeded())
    {
        return false;
    }

    if (!interrupt_map_requested)
    {
        return true;
    }

    return interrupt_map_available &&
           interrupt_map_error ==
               Amg8833DriverError::None;
}

bool Amg8833Snapshot::recoveryAttempted() const
{
    return frame_recovery_attempted ||
           interrupt_map_recovery_attempted;
}

bool Amg8833Snapshot::recoverySucceeded() const
{
    if (!recoveryAttempted())
    {
        return false;
    }

    if (frame_recovery_attempted &&
        !frame_recovery_succeeded)
    {
        return false;
    }

    if (interrupt_map_recovery_attempted &&
        !interrupt_map_recovery_succeeded)
    {
        return false;
    }

    return true;
}

Amg8833SnapshotReader::Amg8833SnapshotReader(
    Amg8833Driver& driver)
    : driver_(driver)
{
}

Amg8833Snapshot
Amg8833SnapshotReader::capture(
    std::uint32_t timestamp_ms,
    bool include_interrupt_map)
{
    Amg8833Snapshot snapshot;

    snapshot.interrupt_map_requested =
        include_interrupt_map;

    const Amg8833Acquisition acquisition =
        driver_.readFrame(
            timestamp_ms);

    snapshot.frame =
        acquisition.frame;

    snapshot.status =
        acquisition.status;

    snapshot.frame_error =
        acquisition.error;

    snapshot.frame_recovery_attempted =
        acquisition.recovery_attempted;

    snapshot.frame_recovery_succeeded =
        acquisition.recovery_succeeded;

    if (acquisition.success())
    {
        snapshot.summary =
            summarize(
                acquisition.frame);
    }

    if (include_interrupt_map &&
        acquisition.success())
    {
        const Amg8833InterruptMapResult
            interrupt_result =
                driver_.readInterruptMap();

        snapshot.interrupt_map =
            interrupt_result.map;

        snapshot.interrupt_map_error =
            interrupt_result.error;

        snapshot.interrupt_map_available =
            interrupt_result.success();

        snapshot.interrupt_map_recovery_attempted =
            interrupt_result.recovery_attempted;

        snapshot.interrupt_map_recovery_succeeded =
            interrupt_result.recovery_succeeded;
    }

    snapshot.health =
        driver_.health();

    return snapshot;
}

Amg8833FrameSummary
Amg8833SnapshotReader::summarize(
    const ThermalFrame& frame)
{
    Amg8833FrameSummary summary;

    summary.thermistor_temperature =
        frame.thermistorTemperature();

    if (!frame.isValid())
    {
        return summary;
    }

    summary.valid_pixel_count =
        FrameStatistics::validPixelCount(
            frame);

    if (summary.valid_pixel_count == 0U)
    {
        return summary;
    }

    summary.minimum_temperature =
        FrameStatistics::minimum(
            frame);

    summary.maximum_temperature =
        FrameStatistics::maximum(
            frame);

    summary.average_temperature =
        FrameStatistics::average(
            frame);

    summary.available = true;

    return summary;
}

}  // namespace leafsense::drivers