#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>

#include "leafsense/processing/dead_pixel_corrector.h"

namespace {

leafsense::ThermalFrame uniformFrame(float value)
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
                value);
        }
    }

    return frame;
}

} // namespace

TEST_CASE("Dead pixel corrector replaces a NaN pixel")
{
    auto frame = uniformFrame(25.0F);
    frame.setPixel(
        3,
        3,
        std::numeric_limits<float>::quiet_NaN());

    leafsense::processing::DeadPixelCorrector corrector;
    const auto result = corrector.applyInPlace(frame);

    REQUIRE(frame.pixel(3, 3) == Catch::Approx(25.0F));
    REQUIRE(result.invalid_pixels_found == 1);
    REQUIRE(result.corrected_pixels == 1);
    REQUIRE(result.uncorrectable_pixels == 0);
}

TEST_CASE("Dead pixel corrector replaces a large local outlier")
{
    auto frame = uniformFrame(25.0F);
    frame.setPixel(4, 4, 60.0F);

    leafsense::processing::DeadPixelCorrector corrector;
    const auto result = corrector.applyInPlace(frame);

    REQUIRE(frame.pixel(4, 4) == Catch::Approx(25.0F));
    REQUIRE(result.outliers_found == 1);
    REQUIRE(result.corrected_pixels == 1);
}

TEST_CASE("Dead pixel correction can be disabled")
{
    auto frame = uniformFrame(25.0F);
    frame.setPixel(4, 4, 60.0F);

    leafsense::processing::DeadPixelCorrectionOptions options;
    options.enabled = false;

    leafsense::processing::DeadPixelCorrector corrector;
    const auto result = corrector.applyInPlace(frame, options);

    REQUIRE(frame.pixel(4, 4) == Catch::Approx(60.0F));
    REQUIRE(result.corrected_pixels == 0);
}

TEST_CASE("Dead pixel corrector leaves a valid temperature gradient")
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
                20.0F + static_cast<float>(x + y));
        }
    }

    leafsense::processing::DeadPixelCorrector corrector;
    const auto result = corrector.applyInPlace(frame);

    REQUIRE(result.corrected_pixels == 0);
}
