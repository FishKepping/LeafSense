#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>

#include "leafsense/processing/temporal_smoother.h"

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

TEST_CASE("First smoothed frame passes through unchanged")
{
    leafsense::processing::TemporalSmoother smoother;
    leafsense::processing::TemporalSmoothingOptions options;
    options.alpha = 0.25F;

    const auto output = smoother.apply(uniformFrame(20.0F), options);

    REQUIRE(output.pixel(0, 0) == Catch::Approx(20.0F));
    REQUIRE(smoother.hasHistory());
}

TEST_CASE("Temporal smoothing applies exponential averaging")
{
    leafsense::processing::TemporalSmoother smoother;
    leafsense::processing::TemporalSmoothingOptions options;
    options.alpha = 0.25F;

    smoother.apply(uniformFrame(20.0F), options);
    const auto output = smoother.apply(uniformFrame(28.0F), options);

    REQUIRE(output.pixel(0, 0) == Catch::Approx(22.0F));
}

TEST_CASE("Temporal smoothing can be disabled")
{
    leafsense::processing::TemporalSmoother smoother;
    leafsense::processing::TemporalSmoothingOptions options;
    options.enabled = false;

    smoother.apply(uniformFrame(20.0F), options);
    const auto output = smoother.apply(uniformFrame(30.0F), options);

    REQUIRE(output.pixel(0, 0) == Catch::Approx(30.0F));
}

TEST_CASE("Reset removes temporal history")
{
    leafsense::processing::TemporalSmoother smoother;
    leafsense::processing::TemporalSmoothingOptions options;
    options.alpha = 0.25F;

    smoother.apply(uniformFrame(20.0F), options);
    smoother.reset();

    const auto output = smoother.apply(uniformFrame(30.0F), options);

    REQUIRE(output.pixel(0, 0) == Catch::Approx(30.0F));
}
