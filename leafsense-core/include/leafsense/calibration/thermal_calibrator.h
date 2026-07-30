#pragma once

#include "leafsense/calibration/calibration_settings.h"
#include "leafsense/thermal_frame.h"

namespace leafsense::calibration {

class ThermalCalibrator
{
public:
    static constexpr float MIN_GAIN = 0.5F;
    static constexpr float MAX_GAIN = 1.5F;
    static constexpr float MIN_OFFSET_C = -20.0F;
    static constexpr float MAX_OFFSET_C = 20.0F;

    ThermalCalibrator() = default;
    explicit ThermalCalibrator(const CalibrationSettings& settings);

    const CalibrationSettings& settings() const;

    bool setSettings(const CalibrationSettings& settings);
    bool setGain(float gain);
    bool setOffset(float offset_c);

    void restoreDefaults();

    float apply(float raw_temperature_c) const;
    ThermalFrame apply(const ThermalFrame& raw_frame) const;
    void applyInPlace(ThermalFrame& frame) const;

    static bool validSettings(const CalibrationSettings& settings);
    static bool validGain(float gain);
    static bool validOffset(float offset_c);

    static float calculateOffset(
        float measured_temperature_c,
        float reference_temperature_c,
        float gain = CalibrationSettings::DEFAULT_GAIN);

private:
    void incrementRevision();

    CalibrationSettings settings_{};
};

} // namespace leafsense::calibration
