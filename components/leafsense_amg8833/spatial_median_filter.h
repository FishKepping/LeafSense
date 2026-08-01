#pragma once

#include "thermal_frame.h"

namespace leafsense::processing {

struct SpatialMedianOptions
{
    bool enabled{false};

    // When true, the centre pixel is included in the 3×3 median.
    bool include_centre{true};
};

class SpatialMedianFilter
{
public:
    ThermalFrame apply(
        const ThermalFrame& frame,
        const SpatialMedianOptions& options = {}) const;
};

} // namespace leafsense::processing
