#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/amg8833_decoder.h"
#include "leafsense/frame_statistics.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/roi/threshold_roi.h"
#include "leafsense/thermal_frame.h"
#include "leafsense/thermal_processor.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::Amg8833Decoder;
using leafsense::FrameStatistics;
using leafsense::ProcessingConfig;
using leafsense::SpatialFilter;
using leafsense::ThermalFrame;
using leafsense::ThermalProcessor;
using leafsense::roi::RectangleRoi;
using leafsense::roi::ThresholdRoi;

constexpr float TEST_TOLERANCE =
    0.0001f;

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

    frame.setPixel(
        x,
        y,
        temperature);
}

ThermalFrame makeUniformFrame(
    float temperature,
    bool valid = true)
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

    frame.setValid(
        valid);

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

    frame.setValid(true);

    return frame;
}

void setRawPixel(
    Amg8833Decoder::PixelBytes& bytes,
    std::size_t pixel_index,
    std::uint16_t raw_value)
{
    const std::size_t byte_index =
        pixel_index * 2;

    bytes[byte_index] =
        static_cast<std::uint8_t>(
            raw_value & 0x00FFU);

    bytes[byte_index + 1] =
        static_cast<std::uint8_t>(
            (raw_value >> 8U) &
            0x000FU);
}

std::uint16_t encodePixelTemperature(
    float temperature)
{
    const auto counts =
        static_cast<std::int16_t>(
            temperature / 0.25f);

    return static_cast<std::uint16_t>(
        counts) &
        0x0FFFU;
}

Amg8833Decoder::PixelBytes makeUniformRawPixels(
    float temperature)
{
    Amg8833Decoder::PixelBytes bytes{};

    const std::uint16_t raw =
        encodePixelTemperature(
            temperature);

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        setRawPixel(
            bytes,
            index,
            raw);
    }

    return bytes;
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

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                first.pixel(x, y),
                WithinAbs(
                    second.pixel(x, y),
                    TEST_TOLERANCE));
        }
    }
}

}  // namespace

TEST_CASE(
    "ThermalProcessor has disabled filters by default",
    "[thermal_processor]")
{
    const ThermalProcessor processor;

    REQUIRE(
        processor.config().spatial_filter ==
        SpatialFilter::None);

    REQUIRE(
        processor.config().spatial_radius ==
        1);

    REQUIRE_FALSE(
        processor.config().exponential_enabled);

    REQUIRE_THAT(
        processor.config().exponential_alpha,
        WithinAbs(
            0.5f,
            TEST_TOLERANCE));

    REQUIRE_FALSE(
        processor.exponentialInitialized());
}

TEST_CASE(
    "ThermalProcessor stores its configuration",
    "[thermal_processor]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    config.spatial_radius =
        2;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.25f;

    const ThermalProcessor processor(
        config);

    REQUIRE(
        processor.config().spatial_filter ==
        SpatialFilter::Median);

    REQUIRE(
        processor.config().spatial_radius ==
        2);

    REQUIRE(
        processor.config().exponential_enabled);

    REQUIRE_THAT(
        processor.config().exponential_alpha,
        WithinAbs(
            0.25f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor clamps exponential alpha",
    "[thermal_processor][validation]")
{
    ProcessingConfig low_config;
    low_config.exponential_alpha =
        -1.0f;

    ProcessingConfig high_config;
    high_config.exponential_alpha =
        2.0f;

    const ThermalProcessor low_processor(
        low_config);

    const ThermalProcessor high_processor(
        high_config);

    REQUIRE_THAT(
        low_processor.config().exponential_alpha,
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        high_processor.config().exponential_alpha,
        WithinAbs(
            1.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor with no filters returns an unchanged frame",
    "[thermal_processor]")
{
    ThermalFrame source =
        makeSequentialFrame();

    source.setFrameNumber(10);
    source.setTimestampMs(1000);
    source.setThermistorTemperature(26.5f);

    ThermalProcessor processor;

    const ThermalFrame output =
        processor.process(
            source);

    requireFramesEqual(
        source,
        output);
}

TEST_CASE(
    "ThermalProcessor applies the mean spatial filter",
    "[thermal_processor][mean]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Mean;

    config.spatial_radius =
        1;

    ThermalProcessor processor(
        config);

    const ThermalFrame source =
        makeSequentialFrame();

    const ThermalFrame output =
        processor.process(
            source);

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            4.5f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor applies the median spatial filter",
    "[thermal_processor][median]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    config.spatial_radius =
        1;

    ThermalProcessor processor(
        config);

    ThermalFrame source =
        makeUniformFrame(
            20.0f);

    source.setPixel(
        3,
        3,
        100.0f);

    const ThermalFrame output =
        processor.process(
            source);

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor applies exponential smoothing",
    "[thermal_processor][exponential]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    const ThermalFrame first =
        makeUniformFrame(
            10.0f);

    const ThermalFrame second =
        makeUniformFrame(
            30.0f);

    const ThermalFrame first_output =
        processor.process(
            first);

    const ThermalFrame second_output =
        processor.process(
            second);

    REQUIRE_THAT(
        first_output.pixel(0, 0),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        second_output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE(
        processor.exponentialInitialized());
}

TEST_CASE(
    "ThermalProcessor applies spatial filtering before temporal filtering",
    "[thermal_processor][pipeline]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    config.spatial_radius =
        1;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    const ThermalFrame first =
        makeUniformFrame(
            20.0f);

    ThermalFrame second =
        makeUniformFrame(
            20.0f);

    second.setPixel(
        3,
        3,
        100.0f);

    processor.process(
        first);

    const ThermalFrame output =
        processor.process(
            second);

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor decodes raw AMG8833 data",
    "[thermal_processor][decoder]")
{
    const auto bytes =
        makeUniformRawPixels(
            25.0f);

    ThermalProcessor processor;

    const ThermalFrame output =
        processor.process(
            bytes,
            0x90,
            0x01,
            123,
            4567,
            true);

    REQUIRE(output.isValid());
    REQUIRE(output.frameNumber() == 123);
    REQUIRE(output.timestampMs() == 4567);

    REQUIRE_THAT(
        output.thermistorTemperature(),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(7, 7),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor filters decoded raw data",
    "[thermal_processor][decoder][median]")
{
    auto bytes =
        makeUniformRawPixels(
            20.0f);

    setRawPixel(
        bytes,
        27,
        encodePixelTemperature(
            100.0f));

    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    config.spatial_radius =
        1;

    ThermalProcessor processor(
        config);

    const ThermalFrame output =
        processor.process(
            bytes,
            0x00,
            0x00,
            1,
            1000,
            true);

    REQUIRE_THAT(
        output.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor preserves current frame metadata",
    "[thermal_processor][metadata]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Mean;

    config.exponential_enabled =
        true;

    ThermalProcessor processor(
        config);

    ThermalFrame first =
        makeUniformFrame(
            10.0f);

    first.setFrameNumber(10);
    first.setTimestampMs(1000);
    first.setThermistorTemperature(20.0f);

    ThermalFrame second =
        makeUniformFrame(
            30.0f);

    second.setFrameNumber(11);
    second.setTimestampMs(1100);
    second.setThermistorTemperature(21.5f);

    processor.process(
        first);

    const ThermalFrame output =
        processor.process(
            second);

    REQUIRE(output.isValid());
    REQUIRE(output.frameNumber() == 11);
    REQUIRE(output.timestampMs() == 1100);

    REQUIRE_THAT(
        output.thermistorTemperature(),
        WithinAbs(
            21.5f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor does not modify its source frame",
    "[thermal_processor]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    config.exponential_enabled =
        true;

    ThermalProcessor processor(
        config);

    ThermalFrame source =
        makeUniformFrame(
            20.0f);

    source.setPixel(
        3,
        3,
        100.0f);

    const ThermalFrame original =
        source;

    processor.process(
        source);

    requireFramesEqual(
        source,
        original);
}

TEST_CASE(
    "Invalid frames pass through unchanged",
    "[thermal_processor][invalid]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Mean;

    config.exponential_enabled =
        true;

    ThermalProcessor processor(
        config);

    ThermalFrame invalid =
        makeUniformFrame(
            30.0f,
            false);

    invalid.setFrameNumber(50);
    invalid.setTimestampMs(5000);

    const ThermalFrame output =
        processor.process(
            invalid);

    requireFramesEqual(
        invalid,
        output);

    REQUIRE_FALSE(
        processor.exponentialInitialized());
}

TEST_CASE(
    "Invalid frames do not advance exponential state",
    "[thermal_processor][invalid][exponential]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    const ThermalFrame first =
        makeUniformFrame(
            10.0f);

    const ThermalFrame invalid =
        makeUniformFrame(
            100.0f,
            false);

    const ThermalFrame third =
        makeUniformFrame(
            30.0f);

    processor.process(
        first);

    processor.process(
        invalid);

    const ThermalFrame output =
        processor.process(
            third);

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "ThermalProcessor reset clears exponential history",
    "[thermal_processor][reset]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    processor.process(
        makeUniformFrame(
            10.0f));

    processor.process(
        makeUniformFrame(
            30.0f));

    REQUIRE(
        processor.exponentialInitialized());

    processor.reset();

    REQUIRE_FALSE(
        processor.exponentialInitialized());

    const ThermalFrame output =
        processor.process(
            makeUniformFrame(
                50.0f));

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            50.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Changing exponential alpha preserves filter history",
    "[thermal_processor][configuration]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    processor.process(
        makeUniformFrame(
            0.0f));

    const ThermalFrame second =
        processor.process(
            makeUniformFrame(
                100.0f));

    REQUIRE_THAT(
        second.pixel(0, 0),
        WithinAbs(
            50.0f,
            TEST_TOLERANCE));

    config.exponential_alpha =
        1.0f;

    processor.setConfig(
        config);

    REQUIRE(
        processor.exponentialInitialized());

    const ThermalFrame third =
        processor.process(
            makeUniformFrame(
                20.0f));

    REQUIRE_THAT(
        third.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Disabling exponential filtering resets its history",
    "[thermal_processor][configuration]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    ThermalProcessor processor(
        config);

    processor.process(
        makeUniformFrame(
            10.0f));

    REQUIRE(
        processor.exponentialInitialized());

    config.exponential_enabled =
        false;

    processor.setConfig(
        config);

    REQUIRE_FALSE(
        processor.exponentialInitialized());
}

TEST_CASE(
    "Re-enabling exponential filtering starts with a fresh frame",
    "[thermal_processor][configuration]")
{
    ProcessingConfig config;

    config.exponential_enabled =
        true;

    config.exponential_alpha =
        0.5f;

    ThermalProcessor processor(
        config);

    processor.process(
        makeUniformFrame(
            10.0f));

    config.exponential_enabled =
        false;

    processor.setConfig(
        config);

    processor.process(
        makeUniformFrame(
            100.0f));

    config.exponential_enabled =
        true;

    processor.setConfig(
        config);

    const ThermalFrame output =
        processor.process(
            makeUniformFrame(
                40.0f));

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            40.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Processed frames work with FrameStatistics",
    "[thermal_processor][integration]")
{
    ProcessingConfig config;

    config.spatial_filter =
        SpatialFilter::Median;

    ThermalProcessor processor(
        config);

    ThermalFrame source =
        makeUniformFrame(
            20.0f);

    source.setPixel(
        3,
        3,
        100.0f);

    const ThermalFrame output =
        processor.process(
            source);

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
    "Processed frames work with RectangleRoi",
    "[thermal_processor][integration]")
{
    ThermalProcessor processor;

    const ThermalFrame output =
        processor.process(
            makeSequentialFrame());

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
            18.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(
            output,
            selection),
        WithinAbs(
            36.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(
            output,
            selection),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Processed frames work with ThresholdRoi",
    "[thermal_processor][integration]")
{
    ThermalProcessor processor;

    const ThermalFrame output =
        processor.process(
            makeSequentialFrame());

    const ThresholdRoi threshold(
        20.0f,
        25.0f);

    const auto selection =
        threshold.selection(
            output);

    REQUIRE(selection.size() == 6);
    REQUIRE(selection[0] == 20);
    REQUIRE(selection[5] == 25);
}