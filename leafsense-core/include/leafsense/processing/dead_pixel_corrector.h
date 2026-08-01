#pragma once

#include <cstddef>

#include "leafsense/thermal_frame.h"

namespace leafsense::processing {

struct DeadPixelCorrectionOptions
{
    bool enabled{true};

    // Values outside this range are considered physically implausible.
    float minimum_temperature_c{-40.0F};
    float maximum_temperature_c{100.0F};

    // A finite pixel is treated as an outlier when it differs from the
    // median of its valid neighbours by more than this amount.
    float neighbour_deviation_threshold_c{8.0F};

    // At least this many valid neighbours are required before correction.
    std::size_t minimum_valid_neighbours{3};
};

struct DeadPixelCorrectionResult
{
    std::size_t invalid_pixels_found{0};
    std::size_t outliers_found{0};
    std::size_t corrected_pixels{0};
    std::size_t uncorrectable_pixels{0};
};

class DeadPixelCorrector
{
public:
    DeadPixelCorrectionResult applyInPlace(
        ThermalFrame& frame,
        const DeadPixelCorrectionOptions& options = {}) const;

    ThermalFrame apply(
        const ThermalFrame& frame,
        DeadPixelCorrectionResult* result = nullptr,
        const DeadPixelCorrectionOptions& options = {}) const;

private:
    static bool validTemperature(
        float temperature,
        const DeadPixelCorrectionOptions& options);

    static bool neighbourMedian(
        const ThermalFrame& frame,
        std::size_t x,
        std::size_t y,
        const DeadPixelCorrectionOptions& options,
        float& median,
        std::size_t& valid_neighbour_count);
};

} // namespace leafsense::processing
