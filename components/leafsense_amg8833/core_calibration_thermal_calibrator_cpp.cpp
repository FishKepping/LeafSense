#include "thermal_calibrator.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace leafsense::calibration {

ThermalCalibrator::ThermalCalibrator(
    const CalibrationSettings& settings)
{
    setSettings(settings);
}

const CalibrationSettings& ThermalCalibrator::settings() const
{
    return settings_;
}

bool ThermalCalibrator::setSettings(
    const CalibrationSettings& settings)
{
    if (!validSettings(settings))
    {
        return false;
    }

    settings_ = settings;
    return true;
}

bool ThermalCalibrator::setGain(float gain)
{
    if (!validGain(gain))
    {
        return false;
    }

    if (settings_.gain != gain)
    {
        settings_.gain = gain;
        incrementRevision();
    }

    return true;
}

bool ThermalCalibrator::setOffset(float offset_c)
{
    if (!validOffset(offset_c))
    {
        return false;
    }

    if (settings_.offset_c != offset_c)
    {
        settings_.offset_c = offset_c;
        incrementRevision();
    }

    return true;
}

void ThermalCalibrator::restoreDefaults()
{
    const std::uint32_t next_revision = settings_.revision + 1U;
    settings_ = CalibrationSettings::defaults();
    settings_.revision = next_revision;
}

float ThermalCalibrator::apply(float raw_temperature_c) const
{
    if (!std::isfinite(raw_temperature_c))
    {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return (raw_temperature_c * settings_.gain) +
           settings_.offset_c;
}

ThermalFrame ThermalCalibrator::apply(
    const ThermalFrame& raw_frame) const
{
    ThermalFrame calibrated_frame = raw_frame;
    applyInPlace(calibrated_frame);
    return calibrated_frame;
}

void ThermalCalibrator::applyInPlace(ThermalFrame& frame) const
{
    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto pixel_x = static_cast<std::uint8_t>(x);
            const auto pixel_y = static_cast<std::uint8_t>(y);

            if (!frame.pixelValid(pixel_x, pixel_y))
            {
                continue;
            }

            frame.setPixel(
                pixel_x,
                pixel_y,
                apply(frame.pixel(pixel_x, pixel_y)));
        }
    }
}

bool ThermalCalibrator::validSettings(
    const CalibrationSettings& settings)
{
    return validGain(settings.gain) &&
           validOffset(settings.offset_c);
}

bool ThermalCalibrator::validGain(float gain)
{
    return std::isfinite(gain) &&
           gain >= MIN_GAIN &&
           gain <= MAX_GAIN;
}

bool ThermalCalibrator::validOffset(float offset_c)
{
    return std::isfinite(offset_c) &&
           offset_c >= MIN_OFFSET_C &&
           offset_c <= MAX_OFFSET_C;
}

float ThermalCalibrator::calculateOffset(
    float measured_temperature_c,
    float reference_temperature_c,
    float gain)
{
    if (!std::isfinite(measured_temperature_c) ||
        !std::isfinite(reference_temperature_c) ||
        !validGain(gain))
    {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return reference_temperature_c -
           (measured_temperature_c * gain);
}

void ThermalCalibrator::incrementRevision()
{
    ++settings_.revision;
}

} // namespace leafsense::calibration
