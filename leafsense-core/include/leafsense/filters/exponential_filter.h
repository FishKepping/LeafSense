#pragma once

#include "leafsense/thermal_frame.h"

namespace leafsense::filters {

/**
 * @brief Applies exponential smoothing across consecutive thermal frames.
 *
 * ExponentialFilter is stateful. Each output frame depends on the
 * previously filtered frame.
 *
 * For every valid pixel:
 *
 *     output =
 *         alpha * current +
 *         (1 - alpha) * previous
 *
 * An alpha of:
 *
 *     1.0 follows the current frame immediately
 *     0.5 equally weights current and previous temperatures
 *     0.0 retains the previously filtered temperatures
 *
 * Alpha values supplied outside the range [0.0, 1.0] are clamped.
 *
 * The first frame supplied after construction or reset passes through
 * unchanged and becomes the filter's initial state.
 *
 * Output metadata is taken from the current source frame:
 *
 *     validity
 *     frame number
 *     timestamp
 *     thermistor temperature
 *
 * The source frame is never modified.
 *
 * The implementation performs no heap allocation.
 */
class ExponentialFilter
{
public:
    /**
     * Construct a filter with alpha 0.5.
     */
    ExponentialFilter();

    /**
     * Construct a filter with the specified smoothing factor.
     */
    explicit ExponentialFilter(float alpha);

    /**
     * Return the configured smoothing factor.
     */
    float alpha() const;

    /**
     * Set the smoothing factor.
     *
     * Values are clamped to the inclusive range [0.0, 1.0].
     */
    void setAlpha(float alpha);

    /**
     * Return true after at least one frame has been applied.
     */
    bool isInitialized() const;

    /**
     * Clear the stored temporal state.
     *
     * The next applied frame will pass through unchanged.
     */
    void reset();

    /**
     * Apply exponential temporal smoothing.
     *
     * The returned frame also becomes the previous filtered frame used
     * by the next call.
     */
    ThermalFrame apply(const ThermalFrame& frame);

private:
    static float clampAlpha(float alpha);

    float alpha_;
    bool initialized_;
    ThermalFrame previous_frame_;
};

}  // namespace leafsense::filters