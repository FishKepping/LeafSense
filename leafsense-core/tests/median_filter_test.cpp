#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/filters/median_filter.h"
#include "leafsense/frame_statistics.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::filters::MedianFilter;
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
    "MedianFilter radius zero produces an unchanged frame",
    "[median_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 0);

    requireFramesEqual(source, filtered);
}

TEST_CASE(
    "MedianFilter preserves a uniform thermal frame",
    "[median_filter]")
{
    const ThermalFrame source =
        makeUniformFrame(24.75f);

    const ThermalFrame filtered =
        MedianFilter::apply(source);

    for (std::size_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::size_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                filtered.pixel(
                    static_cast<std::uint8_t>(x),
                    static_cast<std::uint8_t>(y)),
                WithinAbs(
                    24.75f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "MedianFilter calculates a three-by-three center median",
    "[median_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter calculates an even-sized corner median",
    "[median_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(0, 0),
        WithinAbs(
            4.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter clips the top-right neighbourhood",
    "[median_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(7, 0),
        WithinAbs(
            10.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter clips the bottom-left neighbourhood",
    "[median_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(0, 7),
        WithinAbs(
            52.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter clips the bottom-right neighbourhood",
    "[median_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(7, 7),
        WithinAbs(
            58.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter removes an isolated hot pixel",
    "[median_filter][noise]")
{
    ThermalFrame source =
        makeUniformFrame(20.0f);

    source.setPixel(3, 3, 100.0f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(2, 2),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(4, 4),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter removes an isolated cold pixel",
    "[median_filter][noise]")
{
    ThermalFrame source =
        makeUniformFrame(20.0f);

    source.setPixel(3, 3, -100.0f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter preserves a dominant hot region",
    "[median_filter]")
{
    ThermalFrame source =
        makeUniformFrame(10.0f);

    for (std::uint8_t y = 2; y <= 4; ++y)
    {
        for (std::uint8_t x = 2; x <= 4; ++x)
        {
            source.setPixel(
                x,
                y,
                40.0f);
        }
    }

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            40.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter preserves a sharp vertical edge",
    "[median_filter][edge]")
{
    ThermalFrame source;

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            const float temperature =
                x < 4
                    ? 10.0f
                    : 30.0f;

            source.setPixel(
                x,
                y,
                temperature);
        }
    }

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(2, 3),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(4, 3),
        WithinAbs(
            30.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(5, 3),
        WithinAbs(
            30.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter radius two calculates a five-by-five median",
    "[median_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 2);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter large radius uses the complete frame",
    "[median_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MedianFilter::apply(source, 100);

    for (std::size_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::size_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                filtered.pixel(
                    static_cast<std::uint8_t>(x),
                    static_cast<std::uint8_t>(y)),
                WithinAbs(
                    31.5f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "MedianFilter supports negative temperatures",
    "[median_filter]")
{
    ThermalFrame source =
        makeUniformFrame(-10.0f);

    source.setPixel(3, 3, -100.0f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            -10.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter preserves frame metadata",
    "[median_filter][metadata]")
{
    ThermalFrame source =
        makeSequentialFrame();

    source.setValid(true);
    source.setFrameNumber(1234);
    source.setTimestampMs(987654);
    source.setThermistorTemperature(26.5f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE(filtered.isValid());

    REQUIRE(
        filtered.frameNumber() ==
        1234);

    REQUIRE(
        filtered.timestampMs() ==
        987654);

    REQUIRE_THAT(
        filtered.thermistorTemperature(),
        WithinAbs(
            26.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter preserves an invalid frame state",
    "[median_filter][metadata]")
{
    ThermalFrame source =
        makeSequentialFrame();

    source.setValid(false);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_FALSE(filtered.isValid());
}

TEST_CASE(
    "MedianFilter does not modify the source frame",
    "[median_filter]")
{
    ThermalFrame source =
        makeUniformFrame(10.0f);

    source.setPixel(3, 3, 100.0f);

    const ThermalFrame original = source;

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    requireFramesEqual(
        source,
        original);

    REQUIRE_THAT(
        source.pixel(3, 3),
        WithinAbs(
            100.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter output works with FrameStatistics",
    "[median_filter][frame_statistics]")
{
    ThermalFrame source =
        makeUniformFrame(10.0f);

    source.setPixel(3, 3, 100.0f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    REQUIRE_THAT(
        FrameStatistics::minimum(filtered),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(filtered),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(filtered),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MedianFilter output works with a rectangular ROI",
    "[median_filter][rectangle_roi]")
{
    ThermalFrame source =
        makeUniformFrame(10.0f);

    source.setPixel(3, 3, 100.0f);

    const ThermalFrame filtered =
        MedianFilter::apply(source, 1);

    const RectangleRoi rectangle(
        2,
        2,
        3,
        3);

    const auto selection =
        rectangle.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(
            filtered,
            selection),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(
            filtered,
            selection),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(
            filtered,
            selection),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));
}