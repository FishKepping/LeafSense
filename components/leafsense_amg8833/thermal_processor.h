#pragma once

#include <cstddef>
#include <cstdint>

#include "amg8833_decoder.h"
#include "exponential_filter.h"
#include "thermal_frame.h"

namespace leafsense {

/**
 * @brief Available spatial filtering modes.
 */
enum class SpatialFilter
{
    None,
    Mean,
    Median
};

/**
 * @brief Configuration for ThermalProcessor.
 */
struct ProcessingConfig
{
    SpatialFilter spatial_filter =
        SpatialFilter::None;

    std::size_t spatial_radius =
        1;

    bool exponential_enabled =
        false;

    float exponential_alpha =
        0.5f;
};

/**
 * @brief Decodes and filters AMG8833 thermal frames.
 *
 * ThermalProcessor provides a complete platform-independent processing
 * pipeline:
 *
 *     raw AMG8833 bytes
 *         -> register decoding
 *         -> optional spatial filtering
 *         -> optional exponential temporal filtering
 *         -> ThermalFrame
 *
 * Spatial filtering occurs before temporal filtering.
 *
 * Invalid frames pass through unchanged. They are not spatially filtered
 * and do not initialize or update the exponential filter.
 *
 * ThermalProcessor performs no I2C communication and has no dependency
 * on ESPHome, Arduino or ESP-IDF.
 *
 * The implementation performs no heap allocation.
 */
class ThermalProcessor
{
public:
    /**
     * Construct a processor with default configuration.
     *
     * Defaults:
     *
     *     spatial filter: disabled
     *     spatial radius: 1
     *     exponential filter: disabled
     *     exponential alpha: 0.5
     */
    ThermalProcessor();

    /**
     * Construct a processor with the supplied configuration.
     */
    explicit ThermalProcessor(
        const ProcessingConfig& config);

    /**
     * Return the active normalized configuration.
     */
    const ProcessingConfig& config() const;

    /**
     * Replace the processing configuration.
     *
     * Exponential alpha is clamped to [0.0, 1.0].
     *
     * Enabling or disabling exponential filtering resets its temporal
     * history. Changing alpha while exponential filtering remains
     * enabled preserves the existing history.
     */
    void setConfig(
        const ProcessingConfig& config);

    /**
     * Clear temporal filter history.
     */
    void reset();

    /**
     * Return true when the exponential filter contains previous-frame
     * state.
     */
    bool exponentialInitialized() const;

    /**
     * Process an existing ThermalFrame.
     *
     * The source frame is never modified.
     */
    ThermalFrame process(
        const ThermalFrame& frame);

    /**
     * Decode and process raw AMG8833 register data.
     */
    ThermalFrame process(
        const Amg8833Decoder::PixelBytes& pixel_bytes,
        std::uint8_t thermistor_low_byte,
        std::uint8_t thermistor_high_byte,
        std::uint32_t frame_number,
        std::uint32_t timestamp_ms,
        bool valid = true);

private:
    static ProcessingConfig normalizeConfig(
        const ProcessingConfig& config);

    ProcessingConfig config_;
    filters::ExponentialFilter exponential_filter_;
};

}  // namespace leafsense