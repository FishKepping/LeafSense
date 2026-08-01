#pragma once

#include "leafsense/processing/thermal_processing_pipeline.h"

namespace esphome::leafsense_amg8833 {

class ImageProcessingBridge
{
public:
    leafsense::ThermalFrame process(
        const leafsense::ThermalFrame& calibrated_frame)
    {
        return pipeline_.process(
            calibrated_frame,
            options_,
            &last_result_);
    }

    void reset()
    {
        pipeline_.resetTemporalHistory();
    }

    void set_dead_pixel_enabled(bool enabled)
    {
        options_.dead_pixel.enabled = enabled;
    }

    void set_temporal_enabled(bool enabled)
    {
        options_.temporal.enabled = enabled;
    }

    void set_temporal_alpha(float alpha)
    {
        options_.temporal.alpha = alpha;
    }

    void set_spatial_enabled(bool enabled)
    {
        options_.spatial.enabled = enabled;
    }

    const leafsense::processing::ThermalProcessingResult&
    last_result() const
    {
        return last_result_;
    }

private:
    leafsense::processing::ThermalProcessingPipeline pipeline_{};
    leafsense::processing::ThermalProcessingOptions options_{};
    leafsense::processing::ThermalProcessingResult last_result_{};
};

} // namespace esphome::leafsense_amg8833
