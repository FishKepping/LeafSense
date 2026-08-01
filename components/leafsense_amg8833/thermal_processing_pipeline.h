#pragma once

#include "dead_pixel_corrector.h"
#include "spatial_median_filter.h"
#include "temporal_smoother.h"
#include "thermal_frame.h"

namespace leafsense::processing {

struct ThermalProcessingOptions
{
    DeadPixelCorrectionOptions dead_pixel{};
    TemporalSmoothingOptions temporal{};
    SpatialMedianOptions spatial{};
};

struct ThermalProcessingResult
{
    DeadPixelCorrectionResult dead_pixel{};
};

class ThermalProcessingPipeline
{
public:
    ThermalFrame process(
        const ThermalFrame& calibrated_frame,
        const ThermalProcessingOptions& options,
        ThermalProcessingResult* result = nullptr);

    void resetTemporalHistory();

private:
    DeadPixelCorrector dead_pixel_corrector_{};
    TemporalSmoother temporal_smoother_{};
    SpatialMedianFilter spatial_filter_{};
};

} // namespace leafsense::processing
