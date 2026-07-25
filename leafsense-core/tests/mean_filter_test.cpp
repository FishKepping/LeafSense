#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/filters/mean_filter.h"
#include "leafsense/frame_statistics.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::filters::MeanFilter;
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
    REQUIRE(first.isValid() == second.isValid());

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
            REQUIRE_THAT(
                first.pixel(
                    static_cast<std::uint8_t>(x),
                    static_cast<std::uint8_t>(y)),
                WithinAbs(
                    second.pixel(
                        static_cast<std::uint8_t>(x),
                        static_cast<std::uint8_t>(y)),
                    TEST_TOLERANCE));
        }
    }
}

}  // namespace

TEST_CASE(
    "MeanFilter radius zero produces an unchanged frame",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 0);

    requireFramesEqual(source, filtered);
}

TEST_CASE(
    "MeanFilter preserves a uniform thermal frame",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeUniformFrame(24.75f);

    const ThermalFrame filtered =
        MeanFilter::apply(source);

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
    "MeanFilter calculates a three-by-three center average",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter clips the top-left neighbourhood",
    "[mean_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(0, 0),
        WithinAbs(
            4.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter clips the top-right neighbourhood",
    "[mean_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(7, 0),
        WithinAbs(
            10.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter clips the bottom-left neighbourhood",
    "[mean_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(0, 7),
        WithinAbs(
            52.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter clips the bottom-right neighbourhood",
    "[mean_filter][boundary]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(7, 7),
        WithinAbs(
            58.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter smooths an isolated hot pixel",
    "[mean_filter]")
{
    ThermalFrame source =
        makeUniformFrame(0.0f);

    source.setPixel(3, 3, 90.0f);

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(2, 2),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(4, 4),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(0, 0),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter radius two calculates a five-by-five average",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 2);

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter large radius uses the complete frame",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 100);

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
    "MeanFilter preserves frame metadata",
    "[mean_filter][metadata]")
{
    ThermalFrame source =
        makeSequentialFrame();

    source.setValid(true);
    source.setFrameNumber(1234);
    source.setTimestampMs(987654);
    source.setThermistorTemperature(26.5f);

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

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
    "MeanFilter preserves an invalid frame state",
    "[mean_filter][metadata]")
{
    ThermalFrame source =
        makeSequentialFrame();

    source.setValid(false);

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_FALSE(filtered.isValid());
}

TEST_CASE(
    "MeanFilter does not modify the source frame",
    "[mean_filter]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame original = source;

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    requireFramesEqual(source, original);

    REQUIRE_THAT(
        filtered.pixel(0, 0),
        WithinAbs(
            4.5f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        source.pixel(0, 0),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter output works with FrameStatistics",
    "[mean_filter][frame_statistics]")
{
    ThermalFrame source =
        makeUniformFrame(10.0f);

    source.setPixel(3, 3, 100.0f);

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

    REQUIRE_THAT(
        FrameStatistics::minimum(filtered),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(filtered),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "MeanFilter output works with a rectangular ROI",
    "[mean_filter][rectangle_roi]")
{
    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame filtered =
        MeanFilter::apply(source, 1);

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
            18.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(
            filtered,
            selection),
        WithinAbs(
            36.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(
            filtered,
            selection),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}