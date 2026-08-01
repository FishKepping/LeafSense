#include "dead_pixel_corrector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace leafsense::processing {

bool DeadPixelCorrector::validTemperature(
    float temperature,
    const DeadPixelCorrectionOptions& options)
{
    return std::isfinite(temperature) &&
           temperature >= options.minimum_temperature_c &&
           temperature <= options.maximum_temperature_c;
}

bool DeadPixelCorrector::neighbourMedian(
    const ThermalFrame& frame,
    std::size_t x,
    std::size_t y,
    const DeadPixelCorrectionOptions& options,
    float& median,
    std::size_t& valid_neighbour_count)
{
    std::array<float, 8> neighbours{};
    valid_neighbour_count = 0;

    for (int offset_y = -1; offset_y <= 1; ++offset_y)
    {
        for (int offset_x = -1; offset_x <= 1; ++offset_x)
        {
            if (offset_x == 0 && offset_y == 0)
            {
                continue;
            }

            const int neighbour_x = static_cast<int>(x) + offset_x;
            const int neighbour_y = static_cast<int>(y) + offset_y;

            if (neighbour_x < 0 ||
                neighbour_y < 0 ||
                neighbour_x >= static_cast<int>(ThermalFrame::WIDTH) ||
                neighbour_y >= static_cast<int>(ThermalFrame::HEIGHT))
            {
                continue;
            }

            const float temperature = frame.pixel(
                static_cast<std::uint8_t>(neighbour_x),
                static_cast<std::uint8_t>(neighbour_y));

            if (!validTemperature(temperature, options))
            {
                continue;
            }

            neighbours[valid_neighbour_count++] = temperature;
        }
    }

    if (valid_neighbour_count < options.minimum_valid_neighbours)
    {
        return false;
    }

    for (std::size_t index = 1U;
     index < valid_neighbour_count;
     ++index)
{
    const float value = neighbours[index];
    std::size_t position = index;

    while (position > 0U &&
           neighbours[position - 1U] > value)
    {
        neighbours[position] =
            neighbours[position - 1U];

        --position;
    }

    neighbours[position] = value;
}

    const std::size_t middle = valid_neighbour_count / 2;

    if ((valid_neighbour_count % 2U) == 0U)
    {
        median =
            (neighbours[middle - 1] + neighbours[middle]) * 0.5F;
    }
    else
    {
        median = neighbours[middle];
    }

    return true;
}

DeadPixelCorrectionResult DeadPixelCorrector::applyInPlace(
    ThermalFrame& frame,
    const DeadPixelCorrectionOptions& options) const
{
    DeadPixelCorrectionResult result;

    if (!options.enabled)
    {
        return result;
    }

    // Read neighbours from the original frame so multiple corrections in one
    // pass cannot influence each other.
    const ThermalFrame source = frame;

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);
            const float current = source.pixel(pixel_x, pixel_y);

            float neighbour_median = 0.0F;
            std::size_t valid_neighbour_count = 0;

            const bool has_neighbour_median = neighbourMedian(
                source,
                x,
                y,
                options,
                neighbour_median,
                valid_neighbour_count);

            if (!validTemperature(current, options))
            {
                ++result.invalid_pixels_found;

                if (has_neighbour_median)
                {
                    frame.setPixel(pixel_x, pixel_y, neighbour_median);
                    ++result.corrected_pixels;
                }
                else
                {
                    ++result.uncorrectable_pixels;
                }

                continue;
            }

            if (has_neighbour_median &&
                std::fabs(current - neighbour_median) >
                    options.neighbour_deviation_threshold_c)
            {
                ++result.outliers_found;
                frame.setPixel(pixel_x, pixel_y, neighbour_median);
                ++result.corrected_pixels;
            }
        }
    }

    return result;
}

ThermalFrame DeadPixelCorrector::apply(
    const ThermalFrame& frame,
    DeadPixelCorrectionResult* result,
    const DeadPixelCorrectionOptions& options) const
{
    ThermalFrame corrected = frame;
    const DeadPixelCorrectionResult local_result =
        applyInPlace(corrected, options);

    if (result != nullptr)
    {
        *result = local_result;
    }

    return corrected;
}

} // namespace leafsense::processing
