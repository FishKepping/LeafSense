#include "leafsense/frame_statistics.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace leafsense {

namespace {

/**
 * Returned when no valid temperature values are available.
 *
 * For this initial implementation, an empty frame or selection returns
 * 0.0°C. A richer result type can be introduced later if needed.
 */
constexpr float EMPTY_RESULT = 0.0f;

}  // namespace

float FrameStatistics::minimum(const ThermalFrame& frame)
{
    float minimum_temperature =
        std::numeric_limits<float>::max();

    std::size_t valid_count = 0;

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);

            if (!frame.pixelValid(pixel_x, pixel_y))
            {
                continue;
            }

            const float temperature =
                frame.pixel(pixel_x, pixel_y);

            if (temperature < minimum_temperature)
            {
                minimum_temperature = temperature;
            }

            ++valid_count;
        }
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return minimum_temperature;
}

float FrameStatistics::maximum(const ThermalFrame& frame)
{
    float maximum_temperature =
        std::numeric_limits<float>::lowest();

    std::size_t valid_count = 0;

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);

            if (!frame.pixelValid(pixel_x, pixel_y))
            {
                continue;
            }

            const float temperature =
                frame.pixel(pixel_x, pixel_y);

            if (temperature > maximum_temperature)
            {
                maximum_temperature = temperature;
            }

            ++valid_count;
        }
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return maximum_temperature;
}

float FrameStatistics::average(const ThermalFrame& frame)
{
    double total = 0.0;
    std::size_t valid_count = 0;

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);

            if (!frame.pixelValid(pixel_x, pixel_y))
            {
                continue;
            }

            total += static_cast<double>(
                frame.pixel(pixel_x, pixel_y));

            ++valid_count;
        }
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return static_cast<float>(
        total / static_cast<double>(valid_count));
}

float FrameStatistics::minimum(
    const ThermalFrame& frame,
    const roi::PixelSelection& selection)
{
    float minimum_temperature =
        std::numeric_limits<float>::max();

    std::size_t valid_count = 0;

    for (const std::uint8_t index : selection)
    {
        const std::size_t x =
            static_cast<std::size_t>(index) %
            ThermalFrame::WIDTH;

        const std::size_t y =
            static_cast<std::size_t>(index) /
            ThermalFrame::WIDTH;

        const auto pixel_x = static_cast<std::uint8_t>(x);
        const auto pixel_y = static_cast<std::uint8_t>(y);

        if (!frame.pixelValid(pixel_x, pixel_y))
        {
            continue;
        }

        const float temperature =
            temperatureAtIndex(frame, index);

        if (temperature < minimum_temperature)
        {
            minimum_temperature = temperature;
        }

        ++valid_count;
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return minimum_temperature;
}

float FrameStatistics::maximum(
    const ThermalFrame& frame,
    const roi::PixelSelection& selection)
{
    float maximum_temperature =
        std::numeric_limits<float>::lowest();

    std::size_t valid_count = 0;

    for (const std::uint8_t index : selection)
    {
        const std::size_t x =
            static_cast<std::size_t>(index) %
            ThermalFrame::WIDTH;

        const std::size_t y =
            static_cast<std::size_t>(index) /
            ThermalFrame::WIDTH;

        const auto pixel_x = static_cast<std::uint8_t>(x);
        const auto pixel_y = static_cast<std::uint8_t>(y);

        if (!frame.pixelValid(pixel_x, pixel_y))
        {
            continue;
        }

        const float temperature =
            temperatureAtIndex(frame, index);

        if (temperature > maximum_temperature)
        {
            maximum_temperature = temperature;
        }

        ++valid_count;
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return maximum_temperature;
}

float FrameStatistics::average(
    const ThermalFrame& frame,
    const roi::PixelSelection& selection)
{
    double total = 0.0;
    std::size_t valid_count = 0;

    for (const std::uint8_t index : selection)
    {
        const std::size_t x =
            static_cast<std::size_t>(index) %
            ThermalFrame::WIDTH;

        const std::size_t y =
            static_cast<std::size_t>(index) /
            ThermalFrame::WIDTH;

        const auto pixel_x = static_cast<std::uint8_t>(x);
        const auto pixel_y = static_cast<std::uint8_t>(y);

        if (!frame.pixelValid(pixel_x, pixel_y))
        {
            continue;
        }

        total += static_cast<double>(
            temperatureAtIndex(frame, index));

        ++valid_count;
    }

    if (valid_count == 0)
    {
        return EMPTY_RESULT;
    }

    return static_cast<float>(
        total / static_cast<double>(valid_count));
}

std::size_t FrameStatistics::validPixelCount(
    const ThermalFrame& frame)
{
    return frame.validPixelCount();
}

float FrameStatistics::temperatureAtIndex(
    const ThermalFrame& frame,
    std::uint8_t index)
{
    if (index >= ThermalFrame::PIXEL_COUNT)
    {
        return EMPTY_RESULT;
    }

    const std::size_t x =
        static_cast<std::size_t>(index) %
        ThermalFrame::WIDTH;

    const std::size_t y =
        static_cast<std::size_t>(index) /
        ThermalFrame::WIDTH;

    return frame.pixel(
        static_cast<std::uint8_t>(x),
        static_cast<std::uint8_t>(y));
}

}  // namespace leafsense