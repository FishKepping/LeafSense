#include "leafsense/roi/threshold_roi.h"

#include <cstddef>
#include <cstdint>

namespace leafsense::roi {

ThresholdRoi::ThresholdRoi()
    : minimum_temperature_(0.0f),
      maximum_temperature_(0.0f)
{
}

ThresholdRoi::ThresholdRoi(
    float minimum_temperature,
    float maximum_temperature)
    : minimum_temperature_(minimum_temperature),
      maximum_temperature_(maximum_temperature)
{
}

void ThresholdRoi::setRange(
    float minimum_temperature,
    float maximum_temperature)
{
    minimum_temperature_ = minimum_temperature;
    maximum_temperature_ = maximum_temperature;
}

float ThresholdRoi::minimumTemperature() const
{
    return minimum_temperature_;
}

float ThresholdRoi::maximumTemperature() const
{
    return maximum_temperature_;
}

void ThresholdRoi::setMinimumTemperature(float temperature)
{
    minimum_temperature_ = temperature;
}

void ThresholdRoi::setMaximumTemperature(float temperature)
{
    maximum_temperature_ = temperature;
}

bool ThresholdRoi::isValid() const
{
    return minimum_temperature_ <= maximum_temperature_;
}

bool ThresholdRoi::contains(float temperature) const
{
    if (!isValid())
    {
        return false;
    }

    return temperature >= minimum_temperature_ &&
           temperature <= maximum_temperature_;
}

std::size_t ThresholdRoi::pixelCount(
    const ThermalFrame& frame) const
{
    return selection(frame).size();
}

PixelSelection ThresholdRoi::selection(
    const ThermalFrame& frame) const
{
    PixelSelection result;

    if (!isValid())
    {
        return result;
    }

    for (std::size_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::size_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            const auto pixel_x =
                static_cast<std::uint8_t>(x);

            const auto pixel_y =
                static_cast<std::uint8_t>(y);

            if (!frame.pixelValid(pixel_x, pixel_y))
            {
                continue;
            }

            const float temperature =
                frame.pixel(pixel_x, pixel_y);

            if (!contains(temperature))
            {
                continue;
            }

            const std::size_t index =
                y * ThermalFrame::WIDTH + x;

            result.add(
                static_cast<std::uint8_t>(index));
        }
    }

    return result;
}

}  // namespace leafsense::roi