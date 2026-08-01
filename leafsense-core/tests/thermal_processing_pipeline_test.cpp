#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>

#include "leafsense/processing/thermal_processing_pipeline.h"

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

TEST_CASE("Processing pipeline corrects before smoothing")
{
    leafsense::processing::ThermalProcessingPipeline pipeline;
    leafsense::processing::ThermalProcessingOptions options;

    options.temporal.alpha = 0.5F;
    options.spatial.enabled = false;

    pipeline.process(uniformFrame(20.0F), options);

    auto second = uniformFrame(30.0F);
    second.setPixel(
        3,
        3,
        std::numeric_limits<float>::quiet_NaN());

    leafsense::processing::ThermalProcessingResult result;
    const auto output = pipeline.process(second, options, &result);

    // Dead pixel is corrected to 30, then smoothed with prior 20.
    REQUIRE(output.pixel(3, 3) == Catch::Approx(25.0F));
    REQUIRE(result.dead_pixel.corrected_pixels == 1);
}
