#include "leafsense/processing/temporal_smoother.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace leafsense::processing {

TemporalSmoother::TemporalSmoother()
{
    reset();
}

void TemporalSmoother::reset()
{
    previous_.fill(0.0F);
    invalid_counts_.fill(0);
    initialised_.fill(false);
    has_history_ = false;
}

bool TemporalSmoother::hasHistory() const
{
    return has_history_;
}

bool TemporalSmoother::setAlpha(float alpha)
{
    if (!std::isfinite(alpha) || alpha < 0.0F || alpha > 1.0F)
    {
        return false;
    }

    alpha_ = alpha;
    return true;
}

float TemporalSmoother::alpha() const
{
    return alpha_;
}

bool TemporalSmoother::validTemperature(float temperature)
{
    return std::isfinite(temperature);
}

ThermalFrame TemporalSmoother::apply(
    const ThermalFrame& frame,
    const TemporalSmoothingOptions& options)
{
    ThermalFrame output = frame;

    if (!options.enabled)
    {
        return output;
    }

    if (!setAlpha(options.alpha))
    {
        return output;
    }

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const std::size_t index =
                (y * ThermalFrame::WIDTH) + x;

            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);
            const float current = frame.pixel(pixel_x, pixel_y);

            if (!validTemperature(current))
            {
                ++invalid_counts_[index];

                if (initialised_[index] &&
                    invalid_counts_[index] <
                        options.invalid_reset_threshold)
                {
                    output.setPixel(
                        pixel_x,
                        pixel_y,
                        previous_[index]);
                }
                else
                {
                    initialised_[index] = false;
                    output.setPixel(
                        pixel_x,
                        pixel_y,
                        std::numeric_limits<float>::quiet_NaN());
                }

                continue;
            }

            invalid_counts_[index] = 0;

            if (!initialised_[index])
            {
                previous_[index] = current;
                initialised_[index] = true;
            }
            else
            {
                previous_[index] =
                    (alpha_ * current) +
                    ((1.0F - alpha_) * previous_[index]);
            }

            output.setPixel(pixel_x, pixel_y, previous_[index]);
        }
    }

    has_history_ = std::any_of(
        initialised_.begin(),
        initialised_.end(),
        [](bool value) { return value; });

    return output;
}

} // namespace leafsense::processing
