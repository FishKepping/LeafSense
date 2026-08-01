#pragma once

#include <array>
#include <cstddef>

#include "thermal_frame.h"

namespace leafsense::processing {

struct TemporalSmoothingOptions
{
    bool enabled{true};

    // 1.0 publishes the newest frame unchanged.
    // Smaller values apply more smoothing.
    float alpha{0.35F};

    // Reset a pixel history after this many consecutive invalid samples.
    std::size_t invalid_reset_threshold{3};
};

class TemporalSmoother
{
public:
    TemporalSmoother();

    void reset();
    bool hasHistory() const;

    bool setAlpha(float alpha);
    float alpha() const;

    ThermalFrame apply(
        const ThermalFrame& frame,
        const TemporalSmoothingOptions& options);

private:
    static bool validTemperature(float temperature);

    std::array<float, ThermalFrame::PIXEL_COUNT> previous_{};
    std::array<std::size_t, ThermalFrame::PIXEL_COUNT> invalid_counts_{};
    std::array<bool, ThermalFrame::PIXEL_COUNT> initialised_{};

    float alpha_{0.35F};
    bool has_history_{false};
};

} // namespace leafsense::processing
