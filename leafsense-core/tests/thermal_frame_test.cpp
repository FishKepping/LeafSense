#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "leafsense/thermal_frame.h"

using namespace leafsense;

TEST_CASE("New ThermalFrame starts invalid")
{
    ThermalFrame frame;

    REQUIRE_FALSE(frame.isValid());
}

TEST_CASE("Set and get a pixel")
{
    ThermalFrame frame;

    frame.setPixel(2, 3, 24.5f);

    REQUIRE(frame.pixel(2, 3) == Catch::Approx(24.5f));
}

TEST_CASE("Clear resets pixels")
{
    ThermalFrame frame;

    frame.setPixel(2, 3, 24.5f);

    frame.clear();

    REQUIRE(frame.pixel(2, 3) == Catch::Approx(0.0f));
}

TEST_CASE("Frame number")
{
    ThermalFrame frame;

    frame.setFrameNumber(42);

    REQUIRE(frame.frameNumber() == 42);
}

TEST_CASE("Timestamp")
{
    ThermalFrame frame;

    frame.setTimestampMs(1000);

    REQUIRE(frame.timestampMs() == 1000);
}

TEST_CASE("Thermistor temperature")
{
    ThermalFrame frame;

    frame.setThermistorTemperature(26.8f);

    REQUIRE(frame.thermistorTemperature() == Catch::Approx(26.8f));
}

TEST_CASE("Out of bounds returns zero")
{
    ThermalFrame frame;

    REQUIRE(frame.pixel(100, 100) == Catch::Approx(0.0f));
}