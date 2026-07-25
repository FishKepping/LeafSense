#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/amg8833_decoder.h"
#include "leafsense/filters/exponential_filter.h"
#include "leafsense/filters/median_filter.h"
#include "leafsense/frame_statistics.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/roi/threshold_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::Amg8833Decoder;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::filters::ExponentialFilter;
using leafsense::filters::MedianFilter;
using leafsense::roi::RectangleRoi;
using leafsense::roi::ThresholdRoi;

constexpr float TEST_TOLERANCE =
    0.0001f;

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
            (raw_value >> 8U) & 0x00FFU);
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

std::uint16_t encodeThermistorTemperature(
    float temperature)
{
    const bool negative =
        temperature < 0.0f;

    const float magnitude_temperature =
        negative
            ? -temperature
            : temperature;

    const auto magnitude =
        static_cast<std::uint16_t>(
            magnitude_temperature /
            0.0625f);

    return static_cast<std::uint16_t>(
        (negative ? 0x0800U : 0x0000U) |
        (magnitude & 0x07FFU));
}

Amg8833Decoder::PixelBytes makeUniformRawPixels(
    float temperature)
{
    Amg8833Decoder::PixelBytes bytes{};

    const std::uint16_t raw_value =
        encodePixelTemperature(
            temperature);

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        setRawPixel(
            bytes,
            pixel_index,
            raw_value);
    }

    return bytes;
}

}  // namespace

TEST_CASE(
    "Amg8833Decoder exposes the expected pixel byte count",
    "[amg8833_decoder]")
{
    REQUIRE(
        Amg8833Decoder::PIXEL_BYTE_COUNT ==
        128);

    REQUIRE(
        Amg8833Decoder::PIXEL_BYTE_COUNT ==
        ThermalFrame::PIXEL_COUNT * 2);
}

TEST_CASE(
    "Amg8833Decoder decodes zero pixel temperature",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x00,
            0x00),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes positive pixel temperature",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x64,
            0x00),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes one positive pixel count",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x01,
            0x00),
        WithinAbs(
            0.25f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes one negative pixel count",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0xFF,
            0x0F),
        WithinAbs(
            -0.25f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes negative pixel temperature",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x9C,
            0x0F),
        WithinAbs(
            -25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes documented negative pixel value",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x24,
            0x0F),
        WithinAbs(
            -55.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes maximum positive pixel value",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0xFF,
            0x07),
        WithinAbs(
            511.75f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes minimum negative pixel value",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x00,
            0x08),
        WithinAbs(
            -512.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder ignores unused pixel high-byte bits",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x64,
            0xF0),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder uses low-byte-first pixel ordering",
    "[amg8833_decoder][pixel]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodePixelTemperature(
            0x34,
            0x01),
        WithinAbs(
            77.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes zero thermistor temperature",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x00,
            0x00),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes positive thermistor temperature",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x90,
            0x01),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes one positive thermistor count",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x01,
            0x00),
        WithinAbs(
            0.0625f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes one negative thermistor count",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x01,
            0x08),
        WithinAbs(
            -0.0625f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes negative thermistor temperature",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x90,
            0x09),
        WithinAbs(
            -25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder treats thermistor sign as sign magnitude",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x04,
            0x08),
        WithinAbs(
            -0.25f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes maximum thermistor magnitude",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0xFF,
            0x07),
        WithinAbs(
            127.9375f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0xFF,
            0x0F),
        WithinAbs(
            -127.9375f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder ignores unused thermistor high-byte bits",
    "[amg8833_decoder][thermistor]")
{
    REQUIRE_THAT(
        Amg8833Decoder::decodeThermistorTemperature(
            0x90,
            0xF1),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes a uniform pixel frame",
    "[amg8833_decoder][frame]")
{
    const auto bytes =
        makeUniformRawPixels(
            25.0f);

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    for (std::uint8_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::uint8_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            REQUIRE_THAT(
                frame.pixel(x, y),
                WithinAbs(
                    25.0f,
                    TEST_TOLERANCE));
        }
    }
}

TEST_CASE(
    "Amg8833Decoder preserves row-major register ordering",
    "[amg8833_decoder][frame]")
{
    Amg8833Decoder::PixelBytes bytes{};

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        const float temperature =
            static_cast<float>(pixel_index) *
            0.25f;

        setRawPixel(
            bytes,
            pixel_index,
            encodePixelTemperature(
                temperature));
    }

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    REQUIRE_THAT(
        frame.pixel(0, 0),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(7, 0),
        WithinAbs(
            1.75f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(0, 1),
        WithinAbs(
            2.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(7, 7),
        WithinAbs(
            15.75f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodes mixed positive and negative pixels",
    "[amg8833_decoder][frame]")
{
    auto bytes =
        makeUniformRawPixels(
            0.0f);

    setRawPixel(
        bytes,
        0,
        encodePixelTemperature(
            -25.0f));

    setRawPixel(
        bytes,
        1,
        encodePixelTemperature(
            -0.25f));

    setRawPixel(
        bytes,
        62,
        encodePixelTemperature(
            0.25f));

    setRawPixel(
        bytes,
        63,
        encodePixelTemperature(
            125.0f));

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    REQUIRE_THAT(
        frame.pixel(0, 0),
        WithinAbs(
            -25.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(1, 0),
        WithinAbs(
            -0.25f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(6, 7),
        WithinAbs(
            0.25f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(7, 7),
        WithinAbs(
            125.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder decodePixels retains default metadata",
    "[amg8833_decoder][frame][metadata]")
{
    const auto bytes =
        makeUniformRawPixels(
            20.0f);

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    REQUIRE_FALSE(frame.isValid());
    REQUIRE(frame.frameNumber() == 0);
    REQUIRE(frame.timestampMs() == 0);

    REQUIRE_THAT(
        frame.thermistorTemperature(),
        WithinAbs(
            0.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder builds a complete valid ThermalFrame",
    "[amg8833_decoder][frame][metadata]")
{
    const auto bytes =
        makeUniformRawPixels(
            25.0f);

    const std::uint16_t thermistor_raw =
        encodeThermistorTemperature(
            26.5f);

    const ThermalFrame frame =
        Amg8833Decoder::decodeFrame(
            bytes,
            static_cast<std::uint8_t>(
                thermistor_raw & 0x00FFU),
            static_cast<std::uint8_t>(
                (thermistor_raw >> 8U) &
                0x00FFU),
            1234,
            987654,
            true);

    REQUIRE(frame.isValid());
    REQUIRE(frame.frameNumber() == 1234);
    REQUIRE(frame.timestampMs() == 987654);

    REQUIRE_THAT(
        frame.thermistorTemperature(),
        WithinAbs(
            26.5f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(0, 0),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        frame.pixel(7, 7),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Decoder can mark a decoded frame invalid",
    "[amg8833_decoder][frame][metadata]")
{
    const auto bytes =
        makeUniformRawPixels(
            10.0f);

    const ThermalFrame frame =
        Amg8833Decoder::decodeFrame(
            bytes,
            0x00,
            0x00,
            5,
            100,
            false);

    REQUIRE_FALSE(frame.isValid());
    REQUIRE(frame.frameNumber() == 5);
    REQUIRE(frame.timestampMs() == 100);
}

TEST_CASE(
    "Decoded AMG8833 frame works with FrameStatistics",
    "[amg8833_decoder][integration]")
{
    Amg8833Decoder::PixelBytes bytes{};

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        const float temperature =
            10.0f +
            static_cast<float>(pixel_index) *
                0.25f;

        setRawPixel(
            bytes,
            pixel_index,
            encodePixelTemperature(
                temperature));
    }

    const ThermalFrame frame =
        Amg8833Decoder::decodeFrame(
            bytes,
            0x90,
            0x01,
            1,
            1000,
            true);

    REQUIRE_THAT(
        FrameStatistics::minimum(frame),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame),
        WithinAbs(
            25.75f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame),
        WithinAbs(
            17.875f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Decoded AMG8833 frame works with RectangleRoi",
    "[amg8833_decoder][integration]")
{
    Amg8833Decoder::PixelBytes bytes{};

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        setRawPixel(
            bytes,
            pixel_index,
            encodePixelTemperature(
                static_cast<float>(
                    pixel_index)));
    }

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    const RectangleRoi rectangle(
        2,
        2,
        3,
        3);

    const auto selection =
        rectangle.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(
            frame,
            selection),
        WithinAbs(
            18.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(
            frame,
            selection),
        WithinAbs(
            36.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(
            frame,
            selection),
        WithinAbs(
            27.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Decoded AMG8833 frame works with ThresholdRoi",
    "[amg8833_decoder][integration]")
{
    Amg8833Decoder::PixelBytes bytes{};

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        setRawPixel(
            bytes,
            pixel_index,
            encodePixelTemperature(
                static_cast<float>(
                    pixel_index)));
    }

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    const ThresholdRoi threshold(
        20.0f,
        25.0f);

    const auto selection =
        threshold.selection(
            frame);

    REQUIRE(selection.size() == 6);
    REQUIRE(selection[0] == 20);
    REQUIRE(selection[1] == 21);
    REQUIRE(selection[2] == 22);
    REQUIRE(selection[3] == 23);
    REQUIRE(selection[4] == 24);
    REQUIRE(selection[5] == 25);
}

TEST_CASE(
    "Decoded AMG8833 frame works with MedianFilter",
    "[amg8833_decoder][integration]")
{
    auto bytes =
        makeUniformRawPixels(
            20.0f);

    setRawPixel(
        bytes,
        27,
        encodePixelTemperature(
            100.0f));

    const ThermalFrame frame =
        Amg8833Decoder::decodePixels(
            bytes);

    const ThermalFrame filtered =
        MedianFilter::apply(
            frame,
            1);

    REQUIRE_THAT(
        frame.pixel(3, 3),
        WithinAbs(
            100.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        filtered.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Consecutive decoded frames work with ExponentialFilter",
    "[amg8833_decoder][integration]")
{
    const auto first_bytes =
        makeUniformRawPixels(
            10.0f);

    const auto second_bytes =
        makeUniformRawPixels(
            30.0f);

    const ThermalFrame first =
        Amg8833Decoder::decodeFrame(
            first_bytes,
            0x00,
            0x00,
            1,
            1000,
            true);

    const ThermalFrame second =
        Amg8833Decoder::decodeFrame(
            second_bytes,
            0x00,
            0x00,
            2,
            1100,
            true);

    ExponentialFilter filter(
        0.5f);

    filter.apply(
        first);

    const ThermalFrame output =
        filter.apply(
            second);

    REQUIRE_THAT(
        output.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        output.pixel(7, 7),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE(output.frameNumber() == 2);
    REQUIRE(output.timestampMs() == 1100);
}