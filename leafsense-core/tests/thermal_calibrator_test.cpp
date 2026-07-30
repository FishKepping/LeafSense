#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>

#include "leafsense/calibration/thermal_calibrator.h"

namespace {

leafsense::ThermalFrame makeFrame(float temperature)
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);

    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x)
        {
            frame.setPixel(
                static_cast<std::uint8_t>(x),
                static_cast<std::uint8_t>(y),
                temperature);
        }
    }

    return frame;
}

} // namespace

TEST_CASE("Calibration defaults do not change a temperature")
{
    leafsense::calibration::ThermalCalibrator calibrator;

    REQUIRE(calibrator.apply(24.5F) == Catch::Approx(24.5F));
    REQUIRE(calibrator.settings().gain == Catch::Approx(1.0F));
    REQUIRE(calibrator.settings().offset_c == Catch::Approx(0.0F));
}

TEST_CASE("Calibration applies gain before offset")
{
    leafsense::calibration::CalibrationSettings settings;
    settings.gain = 1.1F;
    settings.offset_c = -2.0F;

    leafsense::calibration::ThermalCalibrator calibrator(settings);

    REQUIRE(calibrator.apply(20.0F) == Catch::Approx(20.0F));
}

TEST_CASE("Calibration applies to an entire thermal frame")
{
    leafsense::ThermalFrame frame = makeFrame(20.0F);

    leafsense::calibration::CalibrationSettings settings;
    settings.gain = 1.0F;
    settings.offset_c = 2.5F;

    leafsense::calibration::ThermalCalibrator calibrator(settings);
    const leafsense::ThermalFrame calibrated = calibrator.apply(frame);

    REQUIRE(calibrated.pixel(0, 0) == Catch::Approx(22.5F));
    REQUIRE(calibrated.pixel(7, 7) == Catch::Approx(22.5F));

    // Original frame remains unchanged.
    REQUIRE(frame.pixel(0, 0) == Catch::Approx(20.0F));
}

TEST_CASE("Calibration preserves invalid pixels")
{
    leafsense::ThermalFrame frame = makeFrame(20.0F);

    frame.setPixel(
        2,
        3,
        std::numeric_limits<float>::quiet_NaN());

    leafsense::calibration::ThermalCalibrator calibrator;

    REQUIRE(calibrator.setOffset(5.0F));

    calibrator.applyInPlace(frame);

    // The invalid NaN value must remain NaN after calibration.
    REQUIRE(std::isnan(frame.pixel(2, 3)));

    // Valid pixels must still receive calibration.
    REQUIRE(frame.pixel(0, 0) == Catch::Approx(25.0F));
}

TEST_CASE("Calibration rejects unsafe gain values")
{
    leafsense::calibration::ThermalCalibrator calibrator;

    REQUIRE_FALSE(calibrator.setGain(0.49F));
    REQUIRE_FALSE(calibrator.setGain(1.51F));
    REQUIRE_FALSE(calibrator.setGain(
        std::numeric_limits<float>::quiet_NaN()));

    REQUIRE(calibrator.settings().gain == Catch::Approx(1.0F));
}

TEST_CASE("Calibration rejects unsafe offset values")
{
    leafsense::calibration::ThermalCalibrator calibrator;

    REQUIRE_FALSE(calibrator.setOffset(-20.1F));
    REQUIRE_FALSE(calibrator.setOffset(20.1F));
    REQUIRE_FALSE(calibrator.setOffset(
        std::numeric_limits<float>::infinity()));

    REQUIRE(calibrator.settings().offset_c == Catch::Approx(0.0F));
}

TEST_CASE("Calibration revision increments when values change")
{
    leafsense::calibration::ThermalCalibrator calibrator;

    REQUIRE(calibrator.settings().revision == 0);

    REQUIRE(calibrator.setOffset(1.0F));
    REQUIRE(calibrator.settings().revision == 1);

    REQUIRE(calibrator.setGain(1.05F));
    REQUIRE(calibrator.settings().revision == 2);

    // Setting the same value does not create a new revision.
    REQUIRE(calibrator.setGain(1.05F));
    REQUIRE(calibrator.settings().revision == 2);
}

TEST_CASE("Restore defaults increments revision")
{
    leafsense::calibration::ThermalCalibrator calibrator;
    calibrator.setGain(1.1F);
    calibrator.setOffset(3.0F);

    const auto previous_revision =
        calibrator.settings().revision;

    calibrator.restoreDefaults();

    REQUIRE(calibrator.settings().gain == Catch::Approx(1.0F));
    REQUIRE(calibrator.settings().offset_c == Catch::Approx(0.0F));
    REQUIRE(calibrator.settings().revision ==
            previous_revision + 1);
}

TEST_CASE("Offset can be calculated from a reference temperature")
{
    const float offset =
        leafsense::calibration::ThermalCalibrator::calculateOffset(
            23.5F,
            25.0F);

    REQUIRE(offset == Catch::Approx(1.5F));
}

TEST_CASE("Offset calculation accounts for gain")
{
    const float offset =
        leafsense::calibration::ThermalCalibrator::calculateOffset(
            20.0F,
            23.0F,
            1.1F);

    REQUIRE(offset == Catch::Approx(1.0F));
}
