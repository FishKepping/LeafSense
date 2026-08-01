#include "leafsense/processing/spatial_median_filter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace leafsense::processing {

ThermalFrame SpatialMedianFilter::apply(
    const ThermalFrame& frame,
    const SpatialMedianOptions& options) const
{
    if (!options.enabled)
    {
        return frame;
    }

    ThermalFrame output = frame;

    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            std::array<float, 9> values{};
            std::size_t count = 0;

            for (int offset_y = -1; offset_y <= 1; ++offset_y)
            {
                for (int offset_x = -1; offset_x <= 1; ++offset_x)
                {
                    if (!options.include_centre &&
                        offset_x == 0 &&
                        offset_y == 0)
                    {
                        continue;
                    }

                    const int sample_x =
                        static_cast<int>(x) + offset_x;
                    const int sample_y =
                        static_cast<int>(y) + offset_y;

                    if (sample_x < 0 ||
                        sample_y < 0 ||
                        sample_x >=
                            static_cast<int>(ThermalFrame::WIDTH) ||
                        sample_y >=
                            static_cast<int>(ThermalFrame::HEIGHT))
                    {
                        continue;
                    }

                    const float value = frame.pixel(
                        static_cast<std::uint8_t>(sample_x),
                        static_cast<std::uint8_t>(sample_y));

                    if (std::isfinite(value))
                    {
                        values[count++] = value;
                    }
                }
            }

            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);

            if (count == 0)
            {
                output.setPixel(
                    pixel_x,
                    pixel_y,
                    std::numeric_limits<float>::quiet_NaN());
                continue;
            }

            std::sort(
                values.begin(),
                values.begin() +
                    static_cast<std::ptrdiff_t>(count));

            const std::size_t middle = count / 2;
            const float median =
                (count % 2U) == 0U
                    ? (values[middle - 1] + values[middle]) * 0.5F
                    : values[middle];

            output.setPixel(pixel_x, pixel_y, median);
        }
    }

    return output;
}

} // namespace leafsense::processing
