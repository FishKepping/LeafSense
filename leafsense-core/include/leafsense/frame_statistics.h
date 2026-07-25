#pragma once

#include <cstddef>
#include <cstdint>

#include "leafsense/roi/pixel_selection.h"
#include "leafsense/thermal_frame.h"

namespace leafsense {

/**
 * @brief Calculates statistics from thermal frames.
 *
 * FrameStatistics contains no state and never modifies its inputs.
 * Calculations can operate on either an entire frame or a selected
 * subset of pixels.
 */
class FrameStatistics
{
public:
    /// Returns the minimum valid temperature in the entire frame.
    static float minimum(const ThermalFrame& frame);

    /// Returns the maximum valid temperature in the entire frame.
    static float maximum(const ThermalFrame& frame);

    /// Returns the average valid temperature in the entire frame.
    static float average(const ThermalFrame& frame);

    /// Returns the minimum valid temperature in a pixel selection.
    static float minimum(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /// Returns the maximum valid temperature in a pixel selection.
    static float maximum(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /// Returns the average valid temperature in a pixel selection.
    static float average(
        const ThermalFrame& frame,
        const roi::PixelSelection& selection);

    /// Returns the number of valid pixels in the frame.
    static std::size_t validPixelCount(
        const ThermalFrame& frame);

private:
    /// Retrieves a temperature using a linear pixel index.
    static float temperatureAtIndex(
        const ThermalFrame& frame,
        std::uint8_t index);
};

}  // namespace leafsense