#pragma once

#include <cmath>
#include <cstdint>

#include "esphome/core/log.h"
#include "esphome/core/preferences.h"

#include "leafsense/calibration/thermal_calibrator.h"

namespace esphome::leafsense_amg8833 {

class CalibrationBridge
{
public:
    static constexpr std::uint32_t PREFERENCE_KEY = 0x4C534336U;

    void setup()
    {
        preference_ =
            global_preferences->make_preference<
                leafsense::calibration::CalibrationSettings>(
                    PREFERENCE_KEY);

        leafsense::calibration::CalibrationSettings stored{};

        if (preference_.load(&stored) &&
            calibrator_.setSettings(stored))
        {
            ESP_LOGI(
                "leafsense.calibration",
                "Loaded calibration: gain %.4f, offset %.3f C, revision %u",
                stored.gain,
                stored.offset_c,
                static_cast<unsigned>(stored.revision));
            return;
        }

        calibrator_.restoreDefaults();
        save();
        ESP_LOGI(
            "leafsense.calibration",
            "Using default calibration");
    }

    bool set_gain(float gain)
    {
        return calibrator_.setGain(gain);
    }

    bool set_offset(float offset_c)
    {
        return calibrator_.setOffset(offset_c);
    }

    bool apply_reference(
        float measured_temperature_c,
        float reference_temperature_c)
    {
        const float offset =
            leafsense::calibration::ThermalCalibrator::calculateOffset(
                measured_temperature_c,
                reference_temperature_c,
                calibrator_.settings().gain);

        return std::isfinite(offset) &&
               calibrator_.setOffset(offset);
    }

    bool save()
    {
        const auto settings = calibrator_.settings();
        const bool saved = preference_.save(&settings);

        if (!saved)
        {
            ESP_LOGE(
                "leafsense.calibration",
                "Failed to save calibration");
        }

        return saved;
    }

    void restore_defaults()
    {
        calibrator_.restoreDefaults();
    }

    float apply(float raw_temperature_c) const
    {
        return calibrator_.apply(raw_temperature_c);
    }

    void apply_in_place(leafsense::ThermalFrame& frame) const
    {
        calibrator_.applyInPlace(frame);
    }

    const leafsense::calibration::CalibrationSettings&
    settings() const
    {
        return calibrator_.settings();
    }

private:
    leafsense::calibration::ThermalCalibrator calibrator_{};
    ESPPreferenceObject preference_{};
};

} // namespace esphome::leafsense_amg8833
