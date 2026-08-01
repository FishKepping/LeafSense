#include "leafsense/filters/median_filter.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace leafsense::filters {

ThermalFrame MedianFilter::apply(
    const ThermalFrame& frame,
    std::size_t radius)
{
    ThermalFrame result = frame;

    constexpr std::size_t maximum_radius =
        (ThermalFrame::WIDTH > ThermalFrame::HEIGHT
             ? ThermalFrame::WIDTH
             : ThermalFrame::HEIGHT) -
        1;

    const std::size_t effective_radius =
        std::min(radius, maximum_radius);

    std::array<float, ThermalFrame::PIXEL_COUNT> samples{};

    for (std::size_t output_y = 0;
         output_y < ThermalFrame::HEIGHT;
         ++output_y)
    {
        for (std::size_t output_x = 0;
             output_x < ThermalFrame::WIDTH;
             ++output_x)
        {
            const std::size_t start_x =
                output_x > effective_radius
                    ? output_x - effective_radius
                    : 0;

            const std::size_t start_y =
                output_y > effective_radius
                    ? output_y - effective_radius
                    : 0;

            const std::size_t end_x =
                std::min(
                    output_x + effective_radius,
                    ThermalFrame::WIDTH - 1);

            const std::size_t end_y =
                std::min(
                    output_y + effective_radius,
                    ThermalFrame::HEIGHT - 1);

            std::size_t sample_count = 0;

            for (std::size_t source_y = start_y;
                 source_y <= end_y;
                 ++source_y)
            {
                for (std::size_t source_x = start_x;
                     source_x <= end_x;
                     ++source_x)
                {
                    const auto pixel_x =
                        static_cast<std::uint8_t>(
                            source_x);

                    const auto pixel_y =
                        static_cast<std::uint8_t>(
                            source_y);

                    if (!frame.pixelValid(
                            pixel_x,
                            pixel_y))
                    {
                        continue;
                    }

                    samples[sample_count] =
                        frame.pixel(
                            pixel_x,
                            pixel_y);

                    ++sample_count;
                }
            }

            float filtered_temperature = 0.0f;

            if (sample_count > 0)
            {
                std::sort(
                    samples.begin(),
                    samples.begin() +
                        static_cast<std::ptrdiff_t>(
                            sample_count));

                const std::size_t middle =
                    sample_count / 2;

                if ((sample_count % 2) == 1)
                {
                    filtered_temperature =
                        samples[middle];
                }
                else
                {
                    const double lower =
                        static_cast<double>(
                            samples[middle - 1]);

                    const double upper =
                        static_cast<double>(
                            samples[middle]);

                    filtered_temperature =
                        static_cast<float>(
                            (lower + upper) / 2.0);
                }
            }

            result.setPixel(
                static_cast<std::uint8_t>(
                    output_x),
                static_cast<std::uint8_t>(
                    output_y),
                filtered_temperature);
        }
    }

    return result;
}

}  // namespace leafsense::filters