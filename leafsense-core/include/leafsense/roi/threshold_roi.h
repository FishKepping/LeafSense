#pragma once

#include <cstddef>

#include "leafsense/roi/pixel_selection.h"
#include "leafsense/thermal_frame.h"

namespace leafsense::roi {

/**
 * @brief Selects thermal pixels whose temperatures lie within a range.
 *
 * ThresholdRoi stores the threshold configuration only. It does not
 * own or copy thermal-frame data.
 *
 * The configured range is inclusive:
 *
 *     minimumTemperature() <= pixel <= maximumTemperature()
 *
 * A ThermalFrame is supplied when generating the PixelSelection.
 *
 * Pixels reported as invalid by ThermalFrame::pixelValid() are ignored.
 */
class ThresholdRoi
{
public:
    /**
     * Construct a threshold range containing only 0 degrees.
     */
    ThresholdRoi();

    /**
     * Construct an inclusive temperature range.
     *
     * @param minimum_temperature Minimum included temperature.
     * @param maximum_temperature Maximum included temperature.
     */
    ThresholdRoi(
        float minimum_temperature,
        float maximum_temperature);

    /**
     * Set both threshold temperatures.
     */
    void setRange(
        float minimum_temperature,
        float maximum_temperature);

    /**
     * Minimum included temperature.
     */
    float minimumTemperature() const;

    /**
     * Maximum included temperature.
     */
    float maximumTemperature() const;

    /**
     * Set the minimum included temperature.
     */
    void setMinimumTemperature(float temperature);

    /**
     * Set the maximum included temperature.
     */
    void setMaximumTemperature(float temperature);

    /**
     * Returns true when the minimum is not greater than the maximum.
     */
    bool isValid() const;

    /**
     * Returns true when a temperature lies inside the configured range.
     */
    bool contains(float temperature) const;

    /**
     * Count matching valid pixels in a thermal frame.
     */
    std::size_t pixelCount(const ThermalFrame& frame) const;

    /**
     * Generate a selection containing matching valid pixels.
     */
    PixelSelection selection(const ThermalFrame& frame) const;

private:
    float minimum_temperature_;
    float maximum_temperature_;
};

}  // namespace leafsense::roi