#pragma once

#include <cstddef>

#include "leafsense/roi/pixel_selection.h"
#include "leafsense/thermal_frame.h"

namespace leafsense {

/**
 * @brief Calculates statistics from thermal frames.
 *
 * Design Notes
 * ------------
 * FrameStatistics contains no state.
 *
 * Every function is static and deterministic.
 *
 * The algorithms never modify a ThermalFrame.
 *
 * A PixelSelection may optionally be supplied to limit the
 * calculations to a subset of pixels.
 *
 * Usage:
 *
 * ThermalFrame frame;
 *
 * float average =
 *     FrameStatistics::average(frame);
 *
 * PixelSelection roi;
 *
 * roi.add(9);
 * roi.add(10);
 *
 * float leafAverage =
 *     FrameStatistics::average(frame, roi);
 */
class FrameStatistics
{
public:

    /**
     * Minimum temperature of the entire frame.
     */
    static float minimum(const ThermalFrame& frame);

    /**
     * Maximum temperature of the entire frame.
     */
    static float maximum(const ThermalFrame& frame);

    /**
     * Average temperature of the entire frame.
     */
    static float average(const ThermalFrame& frame);

    /**
     * Minimum temperature of a pixel selection.
     */
    static float minimum(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /**
     * Maximum temperature of a pixel selection.
     */
    static float maximum(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /**
     * Average temperature of a pixel selection.
     */
    static float average(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /**
     * Returns the number of valid pixels in the frame.
     */
    static std::size_t validPixelCount(
        const ThermalFrame& frame);

private:

    /**
     * Converts a linear pixel index into a temperature.
     *
     * This helper allows all algorithms to work with
     * PixelSelection without exposing implementation details.
     */
    static float temperatureAtIndex(
        const ThermalFrame& frame,
        std::uint8_t index);
};

} // namespace leafsense