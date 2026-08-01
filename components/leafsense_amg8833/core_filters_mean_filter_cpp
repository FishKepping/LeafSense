#include "leafsense/filters/mean_filter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace leafsense::filters {

ThermalFrame MeanFilter::apply(
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

            double sum = 0.0;
            std::size_t valid_count = 0;

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

                    sum += static_cast<double>(
                        frame.pixel(
                            pixel_x,
                            pixel_y));

                    ++valid_count;
                }
            }

            float filtered_temperature = 0.0f;

            if (valid_count > 0)
            {
                filtered_temperature =
                    static_cast<float>(
                        sum /
                        static_cast<double>(
                            valid_count));
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