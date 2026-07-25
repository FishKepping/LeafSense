#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/frame_statistics.h"
#include "leafsense/roi/pixel_selection.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::roi::PixelSelection;

constexpr float TEST_TOLERANCE = 0.0001f;

void setPixelAtIndex(
    ThermalFrame& frame,
    std::size_t index,
    float temperature)
{
    const auto x = static_cast<std::uint8_t>(
        index % ThermalFrame::WIDTH);

    const auto y = static_cast<std::uint8_t>(
        index / ThermalFrame::WIDTH);

    frame.setPixel(x, y, temperature);
}

ThermalFrame makeSequentialFrame()
{
    ThermalFrame frame;

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        setPixelAtIndex(
            frame,
            index,
            static_cast<float>(index));
    }

    return frame;
}

ThermalFrame makeUniformFrame(float temperature)
{
    ThermalFrame frame;

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        setPixelAtIndex(
            frame,
            index,
            temperature);
    }

    return frame;
}

}  // namespace

TEST_CASE(
    "FrameStatistics finds the minimum temperature in a complete frame",
    "[frame_statistics][minimum]")
{
    const ThermalFrame frame = makeSequentialFrame();

    REQUIRE_THAT(
        FrameStatistics::minimum(frame),
        WithinAbs(0.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics finds the maximum temperature in a complete frame",
    "[frame_statistics][maximum]")
{
    const ThermalFrame frame = makeSequentialFrame();

    REQUIRE_THAT(
        FrameStatistics::maximum(frame),
        WithinAbs(63.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics calculates the average temperature of a complete frame",
    "[frame_statistics][average]")
{
    const ThermalFrame frame = makeSequentialFrame();

    REQUIRE_THAT(
        FrameStatistics::average(frame),
        WithinAbs(31.5f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics handles a uniform complete frame",
    "[frame_statistics]")
{
    const ThermalFrame frame = makeUniformFrame(24.75f);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame),
        WithinAbs(24.75f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame),
        WithinAbs(24.75f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame),
        WithinAbs(24.75f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics supports negative temperatures",
    "[frame_statistics]")
{
    ThermalFrame frame = makeUniformFrame(-5.0f);

    setPixelAtIndex(frame, 0, -20.0f);
    setPixelAtIndex(frame, 1, 10.0f);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame),
        WithinAbs(-20.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame),
        WithinAbs(10.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics supports mixed positive and negative temperatures",
    "[frame_statistics]")
{
    ThermalFrame frame = makeUniformFrame(0.0f);

    setPixelAtIndex(frame, 0, -12.5f);
    setPixelAtIndex(frame, 1, 7.5f);
    setPixelAtIndex(frame, 2, 15.0f);
    setPixelAtIndex(frame, 3, -10.0f);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame),
        WithinAbs(-12.5f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame),
        WithinAbs(15.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame),
        WithinAbs(0.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics reports every pixel as valid",
    "[frame_statistics][valid_count]")
{
    const ThermalFrame frame = makeSequentialFrame();

    REQUIRE(
        FrameStatistics::validPixelCount(frame) ==
        ThermalFrame::PIXEL_COUNT);
}

TEST_CASE(
    "FrameStatistics finds the minimum temperature in a selection",
    "[frame_statistics][selection][minimum]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(10));
    REQUIRE(selection.add(25));
    REQUIRE(selection.add(40));

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(10.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics finds the maximum temperature in a selection",
    "[frame_statistics][selection][maximum]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(10));
    REQUIRE(selection.add(25));
    REQUIRE(selection.add(40));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(40.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics calculates the average temperature of a selection",
    "[frame_statistics][selection][average]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(10));
    REQUIRE(selection.add(20));
    REQUIRE(selection.add(30));
    REQUIRE(selection.add(40));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(25.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics handles a single-pixel selection",
    "[frame_statistics][selection]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(37));

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(37.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(37.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(37.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics returns zero for an empty selection",
    "[frame_statistics][selection][empty]")
{
    const ThermalFrame frame = makeSequentialFrame();

    const PixelSelection selection;

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(0.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(0.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(0.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics selection calculations are independent of insertion order",
    "[frame_statistics][selection]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(50));
    REQUIRE(selection.add(10));
    REQUIRE(selection.add(30));

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(10.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(50.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(30.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics selection ignores duplicate insertion attempts",
    "[frame_statistics][selection]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(10));
    REQUIRE_FALSE(selection.add(10));
    REQUIRE(selection.add(20));

    REQUIRE(selection.size() == 2);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(10.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(20.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(15.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "A whole-frame selection matches complete frame statistics",
    "[frame_statistics][selection]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        REQUIRE(
            selection.add(
                static_cast<std::uint8_t>(index)));
    }

    REQUIRE(selection.size() == ThermalFrame::PIXEL_COUNT);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(
            FrameStatistics::minimum(frame),
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(
            FrameStatistics::maximum(frame),
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(
            FrameStatistics::average(frame),
            TEST_TOLERANCE));
}

TEST_CASE(
    "FrameStatistics calculates statistics from corner pixels",
    "[frame_statistics][selection]")
{
    const ThermalFrame frame = makeSequentialFrame();

    PixelSelection selection;

    REQUIRE(selection.add(0));
    REQUIRE(selection.add(7));
    REQUIRE(selection.add(56));
    REQUIRE(selection.add(63));

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(0.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(63.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(31.5f, TEST_TOLERANCE));
}