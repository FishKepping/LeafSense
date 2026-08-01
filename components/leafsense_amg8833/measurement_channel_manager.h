#pragma once

#include <array>
#include <cstddef>

#include "measurement_channel.h"
#include "thermal_frame.h"

namespace leafsense::measurement {

struct MeasurementChannelResult
{
    bool available = false;
    float minimum_temperature = 0.0f;
    float maximum_temperature = 0.0f;
    float average_temperature = 0.0f;
    std::size_t valid_pixel_count = 0U;
};

class MeasurementChannelManager
{
public:
    static constexpr std::size_t CHANNEL_COUNT = 6;

    MeasurementChannelManager();

    void setCalibrationOffset(float offset_celsius);
    float calibrationOffset() const;

    MeasurementChannel& channel(std::size_t index);
    const MeasurementChannel& channel(std::size_t index) const;

    std::array<MeasurementChannelResult, CHANNEL_COUNT> process(
        const ThermalFrame& frame) const;

private:
    MeasurementChannelResult processChannel(
        const ThermalFrame& frame,
        const MeasurementChannel& channel) const;

    std::array<MeasurementChannel, CHANNEL_COUNT> channels_;
    float calibration_offset_celsius_;
};

}  // namespace leafsense::measurement
