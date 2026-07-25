#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/frame_statistics.h"
#include "leafsense/roi/pixel_selection.h"
#include "leafsense/roi/threshold_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::roi::PixelSelection;
using leafsense::roi::ThresholdRoi;

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
    "ThresholdRoi defaults to a zero-only range",
    "[threshold_roi]")
{
    const ThresholdRoi threshold;

    REQUIRE_THAT(
        threshold.minimumTemperature(),
        WithinAbs(0.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        threshold.maximumTemperature(),
        WithinAbs(0.0f, TEST_TOLERANCE));

    REQUIRE(threshold.isValid());
    REQUIRE(threshold.contains(0.0f));
    REQUIRE_FALSE(threshold.contains(-0.1f));
    REQUIRE_FALSE(threshold.contains(0.1f));
}

TEST_CASE(
    "ThresholdRoi stores constructor temperatures",
    "[threshold_roi]")
{
    const ThresholdRoi threshold(20.0f, 30.0f);

    REQUIRE_THAT(
        threshold.minimumTemperature(),
        WithinAbs(20.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        threshold.maximumTemperature(),
        WithinAbs(30.0f, TEST_TOLERANCE));

    REQUIRE(threshold.isValid());
}

TEST_CASE(
    "ThresholdRoi range can be changed",
    "[threshold_roi]")
{
    ThresholdRoi threshold;

    threshold.setRange(15.0f, 25.0f);

    REQUIRE_THAT(
        threshold.minimumTemperature(),
        WithinAbs(15.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        threshold.maximumTemperature(),
        WithinAbs(25.0f, TEST_TOLERANCE));

    threshold.setMinimumTemperature(18.0f);
    threshold.setMaximumTemperature(28.0f);

    REQUIRE_THAT(
        threshold.minimumTemperature(),
        WithinAbs(18.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        threshold.maximumTemperature(),
        WithinAbs(28.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "ThresholdRoi rejects an inverted temperature range",
    "[threshold_roi][validation]")
{
    const ThresholdRoi threshold(30.0f, 20.0f);

    REQUIRE_FALSE(threshold.isValid());
    REQUIRE_FALSE(threshold.contains(25.0f));

    const ThermalFrame frame =
        makeSequentialFrame();

    REQUIRE(threshold.pixelCount(frame) == 0);
    REQUIRE(threshold.selection(frame).empty());
}

TEST_CASE(
    "ThresholdRoi includes both range boundaries",
    "[threshold_roi][contains]")
{
    const ThresholdRoi threshold(20.0f, 30.0f);

    REQUIRE(threshold.contains(20.0f));
    REQUIRE(threshold.contains(25.0f));
    REQUIRE(threshold.contains(30.0f));

    REQUIRE_FALSE(threshold.contains(19.999f));
    REQUIRE_FALSE(threshold.contains(30.001f));
}

TEST_CASE(
    "ThresholdRoi supports negative temperatures",
    "[threshold_roi][contains]")
{
    const ThresholdRoi threshold(-20.0f, -10.0f);

    REQUIRE(threshold.contains(-20.0f));
    REQUIRE(threshold.contains(-15.0f));
    REQUIRE(threshold.contains(-10.0f));

    REQUIRE_FALSE(threshold.contains(-21.0f));
    REQUIRE_FALSE(threshold.contains(-9.0f));
}

TEST_CASE(
    "ThresholdRoi creates a sequential range selection",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(10.0f, 15.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(threshold.pixelCount(frame) == 6);
    REQUIRE(selection.size() == 6);

    REQUIRE(selection[0] == 10);
    REQUIRE(selection[1] == 11);
    REQUIRE(selection[2] == 12);
    REQUIRE(selection[3] == 13);
    REQUIRE(selection[4] == 14);
    REQUIRE(selection[5] == 15);
}

TEST_CASE(
    "ThresholdRoi creates a single-pixel selection",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(37.0f, 37.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(selection.size() == 1);
    REQUIRE(selection[0] == 37);
}

TEST_CASE(
    "ThresholdRoi creates an empty selection when no pixels match",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(100.0f, 200.0f);

    REQUIRE(threshold.pixelCount(frame) == 0);
    REQUIRE(threshold.selection(frame).empty());
}

TEST_CASE(
    "ThresholdRoi creates a full-frame selection",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(0.0f, 63.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(
        selection.size() ==
        ThermalFrame::PIXEL_COUNT);

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        REQUIRE(
            selection[index] ==
            static_cast<std::uint8_t>(index));
    }
}

TEST_CASE(
    "ThresholdRoi selects every matching uniform pixel",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeUniformFrame(24.5f);

    const ThresholdRoi threshold(24.0f, 25.0f);

    REQUIRE(
        threshold.pixelCount(frame) ==
        ThermalFrame::PIXEL_COUNT);
}

TEST_CASE(
    "ThresholdRoi excludes uniform pixels outside the range",
    "[threshold_roi][selection]")
{
    const ThermalFrame frame =
        makeUniformFrame(24.5f);

    const ThresholdRoi threshold(25.0f, 30.0f);

    REQUIRE(threshold.pixelCount(frame) == 0);
}

TEST_CASE(
    "ThresholdRoi preserves row-major pixel ordering",
    "[threshold_roi][selection]")
{
    ThermalFrame frame =
        makeUniformFrame(0.0f);

    setPixelAtIndex(frame, 3, 50.0f);
    setPixelAtIndex(frame, 17, 50.0f);
    setPixelAtIndex(frame, 42, 50.0f);
    setPixelAtIndex(frame, 63, 50.0f);

    const ThresholdRoi threshold(50.0f, 50.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(selection.size() == 4);

    REQUIRE(selection[0] == 3);
    REQUIRE(selection[1] == 17);
    REQUIRE(selection[2] == 42);
    REQUIRE(selection[3] == 63);
}

TEST_CASE(
    "ThresholdRoi selection works with FrameStatistics",
    "[threshold_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(10.0f, 20.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(selection.size() == 11);

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
    "Full-range ThresholdRoi matches whole-frame statistics",
    "[threshold_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const ThresholdRoi threshold(0.0f, 63.0f);

    const PixelSelection selection =
        threshold.selection(frame);

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
    "ThresholdRoi responds to changing frame data",
    "[threshold_roi][selection]")
{
    ThermalFrame frame =
        makeUniformFrame(10.0f);

    const ThresholdRoi threshold(20.0f, 30.0f);

    REQUIRE(threshold.selection(frame).empty());

    setPixelAtIndex(frame, 5, 25.0f);
    setPixelAtIndex(frame, 40, 30.0f);

    const PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(selection.size() == 2);
    REQUIRE(selection[0] == 5);
    REQUIRE(selection[1] == 40);
}

TEST_CASE(
    "ThresholdRoi can be reused with a changed range",
    "[threshold_roi]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    ThresholdRoi threshold(0.0f, 9.0f);

    PixelSelection selection =
        threshold.selection(frame);

    REQUIRE(selection.size() == 10);
    REQUIRE(selection[0] == 0);
    REQUIRE(selection[9] == 9);

    threshold.setRange(60.0f, 63.0f);

    selection = threshold.selection(frame);

    REQUIRE(selection.size() == 4);
    REQUIRE(selection[0] == 60);
    REQUIRE(selection[1] == 61);
    REQUIRE(selection[2] == 62);
    REQUIRE(selection[3] == 63);
}