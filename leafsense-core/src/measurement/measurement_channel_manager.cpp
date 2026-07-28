#include "leafsense/measurement/measurement_channel_manager.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace leafsense::measurement {

MeasurementChannelManager::MeasurementChannelManager()
    : channels_(),
      calibration_offset_celsius_(0.0f)
{
}

void MeasurementChannelManager::setCalibrationOffset(float offset_celsius)
{
    calibration_offset_celsius_ =
        std::isfinite(offset_celsius) ? offset_celsius : 0.0f;
}

float MeasurementChannelManager::calibrationOffset() const
{
    return calibration_offset_celsius_;
}

MeasurementChannel& MeasurementChannelManager::channel(std::size_t index)
{
    if (index >= CHANNEL_COUNT)
    {
        throw std::out_of_range("measurement channel index");
    }
    return channels_[index];
}

const MeasurementChannel& MeasurementChannelManager::channel(
    std::size_t index) const
{
    if (index >= CHANNEL_COUNT)
    {
        throw std::out_of_range("measurement channel index");
    }
    return channels_[index];
}

std::array<MeasurementChannelResult, MeasurementChannelManager::CHANNEL_COUNT>
MeasurementChannelManager::process(const ThermalFrame& frame) const
{
    std::array<MeasurementChannelResult, CHANNEL_COUNT> results{};

    for (std::size_t index = 0U; index < CHANNEL_COUNT; ++index)
    {
        results[index] = processChannel(frame, channels_[index]);
    }

    return results;
}

MeasurementChannelResult MeasurementChannelManager::processChannel(
    const ThermalFrame& frame,
    const MeasurementChannel& channel) const
{
    MeasurementChannelResult result{};

    if (!frame.isValid() || !channel.enabled())
    {
        return result;
    }

    const roi::PixelSelection selection = channel.selection();
    if (selection.empty())
    {
        return result;
    }

    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();
    float sum = 0.0f;
    std::size_t count = 0U;

    for (const std::uint8_t linear_index : selection)
    {
        const std::uint8_t x = static_cast<std::uint8_t>(
            linear_index % ThermalFrame::WIDTH);
        const std::uint8_t y = static_cast<std::uint8_t>(
            linear_index / ThermalFrame::WIDTH);

        if (!frame.pixelValid(x, y))
        {
            continue;
        }

        const float temperature =
            frame.pixel(x, y) + calibration_offset_celsius_;

        if (!std::isfinite(temperature))
        {
            continue;
        }

        minimum = temperature < minimum ? temperature : minimum;
        maximum = temperature > maximum ? temperature : maximum;
        sum += temperature;
        ++count;
    }

    if (count == 0U)
    {
        return result;
    }

    result.available = true;
    result.minimum_temperature = minimum;
    result.maximum_temperature = maximum;
    result.average_temperature = sum / static_cast<float>(count);
    result.valid_pixel_count = count;
    return result;
}

}  // namespace leafsense::measurement
