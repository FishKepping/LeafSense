#pragma once

#include <cstddef>

#include "thermal_frame.h"

namespace leafsense::filters {

/**
 * @brief Applies a spatial median filter to a thermal frame.
 *
 * Each output pixel is calculated from the valid source pixels within
 * a square neighbourhood around the corresponding input pixel.
 *
 * A radius of:
 *
 *     0 produces a 1x1 neighbourhood
 *     1 produces a 3x3 neighbourhood
 *     2 produces a 5x5 neighbourhood
 *
 * Neighbourhoods are clipped at the thermal-frame boundaries.
 *
 * Invalid source pixels are ignored. If a neighbourhood contains no
 * valid pixels, the output temperature is set to 0.0.
 *
 * For an odd number of valid samples, the middle sorted value is used.
 *
 * For an even number of valid samples, the two middle sorted values
 * are averaged.
 *
 * The returned ThermalFrame preserves the source frame's:
 *
 *     validity
 *     frame number
 *     timestamp
 *     thermistor temperature
 *
 * The source frame is never modified.
 *
 * The implementation uses fixed-capacity storage and performs no heap
 * allocation.
 */
class MedianFilter
{
public:
    /**
     * Apply a spatial median filter.
     *
     * @param frame Source thermal frame.
     * @param radius Neighbourhood radius.
     *
     * @return A filtered copy of the source frame.
     */
    static ThermalFrame apply(
        const ThermalFrame& frame,
        std::size_t radius = 1);
};

}  // namespace leafsense::filters