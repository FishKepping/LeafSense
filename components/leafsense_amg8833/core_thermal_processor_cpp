#include "leafsense/thermal_processor.h"

#include "leafsense/filters/mean_filter.h"
#include "leafsense/filters/median_filter.h"

namespace leafsense {

ThermalProcessor::ThermalProcessor()
    : config_(
          normalizeConfig(
              ProcessingConfig{})),
      exponential_filter_(
          config_.exponential_alpha)
{
}

ThermalProcessor::ThermalProcessor(
    const ProcessingConfig& config)
    : config_(
          normalizeConfig(
              config)),
      exponential_filter_(
          config_.exponential_alpha)
{
}

const ProcessingConfig&
ThermalProcessor::config() const
{
    return config_;
}

void ThermalProcessor::setConfig(
    const ProcessingConfig& config)
{
    const ProcessingConfig normalized =
        normalizeConfig(
            config);

    const bool exponential_mode_changed =
        normalized.exponential_enabled !=
        config_.exponential_enabled;

    config_ =
        normalized;

    exponential_filter_.setAlpha(
        config_.exponential_alpha);

    if (exponential_mode_changed)
    {
        exponential_filter_.reset();
    }
}

void ThermalProcessor::reset()
{
    exponential_filter_.reset();
}

bool ThermalProcessor::exponentialInitialized() const
{
    return exponential_filter_.isInitialized();
}

ThermalFrame ThermalProcessor::process(
    const ThermalFrame& frame)
{
    if (!frame.isValid())
    {
        return frame;
    }

    ThermalFrame result =
        frame;

    switch (config_.spatial_filter)
    {
        case SpatialFilter::Mean:
            result =
                filters::MeanFilter::apply(
                    result,
                    config_.spatial_radius);
            break;

        case SpatialFilter::Median:
            result =
                filters::MedianFilter::apply(
                    result,
                    config_.spatial_radius);
            break;

        case SpatialFilter::None:
        default:
            break;
    }

    if (config_.exponential_enabled)
    {
        result =
            exponential_filter_.apply(
                result);
    }

    return result;
}

ThermalFrame ThermalProcessor::process(
    const Amg8833Decoder::PixelBytes& pixel_bytes,
    std::uint8_t thermistor_low_byte,
    std::uint8_t thermistor_high_byte,
    std::uint32_t frame_number,
    std::uint32_t timestamp_ms,
    bool valid)
{
    const ThermalFrame decoded =
        Amg8833Decoder::decodeFrame(
            pixel_bytes,
            thermistor_low_byte,
            thermistor_high_byte,
            frame_number,
            timestamp_ms,
            valid);

    return process(
        decoded);
}

ProcessingConfig ThermalProcessor::normalizeConfig(
    const ProcessingConfig& config)
{
    ProcessingConfig normalized =
        config;

    if (normalized.exponential_alpha < 0.0f)
    {
        normalized.exponential_alpha =
            0.0f;
    }
    else if (normalized.exponential_alpha > 1.0f)
    {
        normalized.exponential_alpha =
            1.0f;
    }

    return normalized;
}

}  // namespace leafsense