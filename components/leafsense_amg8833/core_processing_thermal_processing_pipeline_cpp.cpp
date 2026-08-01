#include "thermal_processing_pipeline.h"

namespace leafsense::processing {

ThermalFrame ThermalProcessingPipeline::process(
    const ThermalFrame& calibrated_frame,
    const ThermalProcessingOptions& options,
    ThermalProcessingResult* result)
{
    ThermalProcessingResult local_result;

    ThermalFrame processed =
        dead_pixel_corrector_.apply(
            calibrated_frame,
            &local_result.dead_pixel,
            options.dead_pixel);

    processed = temporal_smoother_.apply(
        processed,
        options.temporal);

    processed = spatial_filter_.apply(
        processed,
        options.spatial);

    if (result != nullptr)
    {
        *result = local_result;
    }

    return processed;
}

void ThermalProcessingPipeline::resetTemporalHistory()
{
    temporal_smoother_.reset();
}

} // namespace leafsense::processing
