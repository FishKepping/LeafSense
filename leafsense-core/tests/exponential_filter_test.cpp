#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/filters/exponential_filter.h"
#include "leafsense/frame_statistics.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::filters::ExponentialFilter;
using leafsense::roi::RectangleRoi;

constexpr float TEST_TOLERANCE = 0.0001f;

void setPixelAtIndex(
    ThermalFrame& frame,
    std::size_t index,
    float temperature)
{
    const auto x =
        static_cast<std::uint8_t>(
            index % ThermalFrame::WIDTH);

    const auto y =
        static_cast<std::uint8_t>(
            index / ThermalFrame::WIDTH);

    frame.setPixel(x, y, temperature);
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

void requireFramesEqual(
    const ThermalFrame& first,
    const ThermalFrame& second)
{
    REQUIRE(
        first.isValid() ==
        second.isValid());

    REQUIRE(
        first.frameNumber() ==
        second.frameNumber());

    REQUIRE(
        first.timestampMs() ==
        second.timestampMs());

    REQUIRE_THAT(
        first.thermistorTemperature(),
        WithinAbs(
            second.thermistorTemperature(),
            TEST_TOLERANCE));

    for (std::size_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::size_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            const auto pixel_x =
                static_cast<std::uint8_t>(x);

            const auto pixel_y =
                static_cast<std::uint8_t>(y);

            REQUIRE_THAT(
                first.pixel(
                    pixel_x,
                    pixel_y),
                WithinAbs(
                    second.pixel(
                        pixel_x,
                        pixel_y),
                    TEST_TOLERANCE));
        }
    }
}

}  // namespace

TEST_CASE(
    "ExponentialFilter defaults to alpha one-half",
    "[exponential_filter]")
{
    const ExponentialFilter filter;

    REQUIRE_THAT(
        filter.alpha(),
        WithinAbs(
            0.5f,
            TEST_TOLERANCE));

    REQUIRE_FALSE(filter.isInitialized());
}

TEST_CASE(
    "ExponentialFilter stores constructor alpha",
    "[exponential_filter]")
{
    const ExponentialFilter filter(0.25f);

    REQUIRE_THAT(
        filter.alpha(),
        WithinAbs(
            0.25f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter clamps constructor alpha",
    "[exponential_filter][validation]")
{
    const ExponentialFilter below_range(-1.0f);
    const ExponentialFilter above_range(2.0f);

    REQUIRE_THAT(
        below_range.alpha(),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        above_range.alpha(),
        WithinAbs(
            1.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter alpha can be changed",
    "[exponential_filter]")
{
    ExponentialFilter filter;

    filter.setAlpha(0.75f);

    REQUIRE_THAT(
        filter.alpha(),
        WithinAbs(
            0.75f,
            TEST_TOLERANCE));

    filter.setAlpha(-10.0f);

    REQUIRE_THAT(
        filter.alpha(),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));

    filter.setAlpha(10.0f);

    REQUIRE_THAT(
        filter.alpha(),
        WithinAbs(
            1.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter passes the first frame through unchanged",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.25f);

    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame output =
        filter.apply(source);

    requireFramesEqual(source, output);
    REQUIRE(filter.isInitialized());
}

TEST_CASE(
    "ExponentialFilter alpha one follows the current frame",
    "[exponential_filter]")
{
    ExponentialFilter filter(1.0f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(50.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                output.pixel(x, y),
                WithinAbs(
                    50.0f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "ExponentialFilter alpha zero retains previous temperatures",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.0f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(50.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                output.pixel(x, y),
                WithinAbs(
                    10.0f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "ExponentialFilter alpha one-half averages two frames",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(30.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                output.pixel(x, y),
                WithinAbs(
                    20.0f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "ExponentialFilter uses the previous filtered output",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(0.0f);

    const ThermalFrame second =
        makeUniformFrame(100.0f);

    const ThermalFrame third =
        makeUniformFrame(100.0f);

    filter.apply(first);

    const ThermalFrame second_output =
        filter.apply(second);

    const ThermalFrame third_output =
        filter.apply(third);

    REQUIRE_THAT(
        second_output.pixel(3, 3),
        WithinAbs(
            50.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        third_output.pixel(3, 3),
        WithinAbs(
            75.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter smooths an isolated changing pixel",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.25f);

    ThermalFrame first =
        makeUniformFrame(20.0f);

    ThermalFrame second =
        makeUniformFrame(20.0f);

    second.setPixel(
        3,
        3,
        100.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            40.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter operates independently on every pixel",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    ThermalFrame first =
        makeUniformFrame(0.0f);

    ThermalFrame second =
        makeUniformFrame(0.0f);

    first.setPixel(0, 0, 10.0f);
    first.setPixel(7, 7, 20.0f);

    second.setPixel(0, 0, 30.0f);
    second.setPixel(7, 7, 60.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(7, 7),
        WithinAbs(
            40.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter preserves current frame metadata",
    "[exponential_filter][metadata]")
{
    ExponentialFilter filter(0.5f);

    ThermalFrame first =
        makeUniformFrame(10.0f);

    first.setValid(true);
    first.setFrameNumber(100);
    first.setTimestampMs(1000);
    first.setThermistorTemperature(20.0f);

    ThermalFrame second =
        makeUniformFrame(30.0f);

    second.setValid(false);
    second.setFrameNumber(101);
    second.setTimestampMs(1100);
    second.setThermistorTemperature(21.5f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    REQUIRE_FALSE(output.isValid());

    REQUIRE(
        output.frameNumber() ==
        101);

    REQUIRE(
        output.timestampMs() ==
        1100);

    REQUIRE_THAT(
        output.thermistorTemperature(),
        WithinAbs(
            21.5f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter does not modify source frames",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(30.0f);

    const ThermalFrame first_original =
        first;

    const ThermalFrame second_original =
        second;

    filter.apply(first);
    filter.apply(second);

    requireFramesEqual(
        first,
        first_original);

    requireFramesEqual(
        second,
        second_original);
}

TEST_CASE(
    "ExponentialFilter reset clears temporal state",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(30.0f);

    filter.apply(first);
    filter.apply(second);

    REQUIRE(filter.isInitialized());

    filter.reset();

    REQUIRE_FALSE(filter.isInitialized());

    const ThermalFrame output =
        filter.apply(second);

    requireFramesEqual(
        output,
        second);

    REQUIRE(filter.isInitialized());
}

TEST_CASE(
    "ExponentialFilter supports changing alpha while initialized",
    "[exponential_filter]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(0.0f);

    const ThermalFrame second =
        makeUniformFrame(100.0f);

    const ThermalFrame third =
        makeUniformFrame(20.0f);

    filter.apply(first);

    const ThermalFrame second_output =
        filter.apply(second);

    REQUIRE_THAT(
        second_output.pixel(0, 0),
        WithinAbs(
            50.0f,
            TEST_TOLERANCE));

    filter.setAlpha(1.0f);

    const ThermalFrame third_output =
        filter.apply(third);

    REQUIRE_THAT(
        third_output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter output works with FrameStatistics",
    "[exponential_filter][frame_statistics]")
{
    ExponentialFilter filter(0.5f);

    const ThermalFrame first =
        makeUniformFrame(10.0f);

    const ThermalFrame second =
        makeUniformFrame(30.0f);

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    REQUIRE_THAT(
        FrameStatistics::minimum(output),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(output),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(output),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ExponentialFilter output works with a rectangular ROI",
    "[exponential_filter][rectangle_roi]")
{
    ExponentialFilter filter(0.5f);

    ThermalFrame first =
        makeUniformFrame(10.0f);

    ThermalFrame second =
        makeUniformFrame(10.0f);

    for (std::uint8_t y = 2; y <= 4; ++y)
    {
        for (std::uint8_t x = 2; x <= 4; ++x)
        {
            second.setPixel(
                x,
                y,
                30.0f);
        }
    }

    filter.apply(first);

    const ThermalFrame output =
        filter.apply(second);

    const RectangleRoi rectangle(
        2,
        2,
        3,
        3);

    const auto selection =
        rectangle.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(
            output,
            selection),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(
            output,
            selection),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(
            output,
            selection),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}