#include "exponential_filter.h"

#include <cstddef>
#include <cstdint>

namespace leafsense::filters {

ExponentialFilter::ExponentialFilter()
    : alpha_(0.5f),
      initialized_(false),
      previous_frame_()
{
}

ExponentialFilter::ExponentialFilter(float alpha)
    : alpha_(clampAlpha(alpha)),
      initialized_(false),
      previous_frame_()
{
}

float ExponentialFilter::alpha() const
{
    return alpha_;
}

void ExponentialFilter::setAlpha(float alpha)
{
    alpha_ = clampAlpha(alpha);
}

bool ExponentialFilter::isInitialized() const
{
    return initialized_;
}

void ExponentialFilter::reset()
{
    initialized_ = false;
    previous_frame_.clear();
}

ThermalFrame ExponentialFilter::apply(
    const ThermalFrame& frame)
{
    if (!initialized_)
    {
        previous_frame_ = frame;
        initialized_ = true;

        return frame;
    }

    ThermalFrame result = frame;

    const float previous_weight =
        1.0f - alpha_;

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

            const bool current_valid =
                frame.pixelValid(
                    pixel_x,
                    pixel_y);

            const bool previous_valid =
                previous_frame_.pixelValid(
                    pixel_x,
                    pixel_y);

            float filtered_temperature = 0.0f;

            if (current_valid && previous_valid)
            {
                const float current_temperature =
                    frame.pixel(
                        pixel_x,
                        pixel_y);

                const float previous_temperature =
                    previous_frame_.pixel(
                        pixel_x,
                        pixel_y);

                filtered_temperature =
                    alpha_ * current_temperature +
                    previous_weight *
                        previous_temperature;
            }
            else if (current_valid)
            {
                filtered_temperature =
                    frame.pixel(
                        pixel_x,
                        pixel_y);
            }
            else if (previous_valid)
            {
                filtered_temperature =
                    previous_frame_.pixel(
                        pixel_x,
                        pixel_y);
            }

            result.setPixel(
                pixel_x,
                pixel_y,
                filtered_temperature);
        }
    }

    previous_frame_ = result;

    return result;
}

float ExponentialFilter::clampAlpha(float alpha)
{
    if (alpha < 0.0f)
    {
        return 0.0f;
    }

    if (alpha > 1.0f)
    {
        return 1.0f;
    }

    return alpha;
}

}  // namespace leafsense::filters