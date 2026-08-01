#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "leafsense/measurement/measurement_channel_manager.h"

namespace {

using Catch::Approx;
using leafsense::ThermalFrame;
using leafsense::measurement::MeasurementChannelManager;
using leafsense::measurement::MeasurementChannelType;
using leafsense::measurement::MeasurementPoint;
using leafsense::measurement::MeasurementRectangle;

ThermalFrame makeFrame()
{
    ThermalFrame frame;
    frame.setValid(true);

    for (std::uint8_t y = 0U; y < 8U; ++y)
    {
        for (std::uint8_t x = 0U; x < 8U; ++x)
        {
            frame.setPixel(x, y, static_cast<float>(y * 8U + x));
        }
    }

    return frame;
}

}  // namespace

TEST_CASE("manager exposes six fixed disabled channels")
{
    MeasurementChannelManager manager;

    for (std::size_t index = 0U;
         index < MeasurementChannelManager::CHANNEL_COUNT;
         ++index)
    {
        REQUIRE_FALSE(manager.channel(index).enabled());
        REQUIRE(manager.channel(index).type() == MeasurementChannelType::Disabled);
    }
}

TEST_CASE("rectangle channel uses pixel centres")
{
    MeasurementChannelManager manager;
    REQUIRE(manager.channel(0).setRectangle({0.0f, 0.0f, 2.0f, 2.0f}));

    const auto results = manager.process(makeFrame());
    REQUIRE(results[0].available);
    REQUIRE(results[0].valid_pixel_count == 4U);
    REQUIRE(results[0].minimum_temperature == Approx(0.0f));
    REQUIRE(results[0].maximum_temperature == Approx(9.0f));
    REQUIRE(results[0].average_temperature == Approx(4.5f));
}

TEST_CASE("polygon and rectangle channels can be mixed")
{
    MeasurementChannelManager manager;
    REQUIRE(manager.channel(0).setRectangle({0.0f, 0.0f, 1.0f, 1.0f}));

    const MeasurementPoint polygon[] = {
        {4.0f, 4.0f},
        {8.0f, 4.0f},
        {8.0f, 8.0f},
        {4.0f, 8.0f},
    };
    REQUIRE(manager.channel(1).setPolygon(polygon, 4U));

    const auto results = manager.process(makeFrame());
    REQUIRE(results[0].available);
    REQUIRE(results[1].available);
    REQUIRE(results[1].valid_pixel_count == 16U);
    REQUIRE_FALSE(results[2].available);
}

TEST_CASE("sensor-wide calibration is applied before every channel result")
{
    MeasurementChannelManager manager;
    manager.setCalibrationOffset(-1.5f);
    REQUIRE(manager.channel(0).setRectangle({0.0f, 0.0f, 1.0f, 1.0f}));
    REQUIRE(manager.channel(1).setRectangle({1.0f, 0.0f, 1.0f, 1.0f}));

    const auto results = manager.process(makeFrame());
    REQUIRE(results[0].minimum_temperature == Approx(-1.5f));
    REQUIRE(results[1].minimum_temperature == Approx(-0.5f));
}

TEST_CASE("disabled channel reports unavailable")
{
    MeasurementChannelManager manager;
    REQUIRE(manager.channel(0).setRectangle({0.0f, 0.0f, 2.0f, 2.0f}));
    manager.channel(0).disable();

    const auto results = manager.process(makeFrame());
    REQUIRE_FALSE(results[0].available);
    REQUIRE(results[0].valid_pixel_count == 0U);
}

TEST_CASE("pixel mask channel processes exactly the selected sensor cells")
{
    MeasurementChannelManager manager;
    leafsense::roi::PixelSelection selection;
    REQUIRE(selection.add(0U));
    REQUIRE(selection.add(9U));
    REQUIRE(selection.add(63U));
    REQUIRE(manager.channel(0).setPixelSelection(selection));

    const auto results = manager.process(makeFrame());
    REQUIRE(manager.channel(0).type() == MeasurementChannelType::PixelMask);
    REQUIRE(results[0].available);
    REQUIRE(results[0].valid_pixel_count == 3U);
    REQUIRE(results[0].minimum_temperature == Approx(0.0f));
    REQUIRE(results[0].maximum_temperature == Approx(63.0f));
    REQUIRE(results[0].average_temperature == Approx(24.0f));
}
