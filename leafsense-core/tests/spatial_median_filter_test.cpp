#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>

#include "leafsense/processing/spatial_median_filter.h"

TEST_CASE("Spatial median filter removes an isolated spike")
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
                25.0F);
        }
    }

    frame.setPixel(3, 3, 70.0F);

    leafsense::processing::SpatialMedianOptions options;
    options.enabled = true;

    leafsense::processing::SpatialMedianFilter filter;
    const auto output = filter.apply(frame, options);

    REQUIRE(output.pixel(3, 3) == Catch::Approx(25.0F));
}

TEST_CASE("Spatial median filter is disabled by default")
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);
    frame.setPixel(3, 3, 70.0F);

    leafsense::processing::SpatialMedianFilter filter;
    const auto output = filter.apply(frame);

    REQUIRE(output.pixel(3, 3) == Catch::Approx(70.0F));
}
