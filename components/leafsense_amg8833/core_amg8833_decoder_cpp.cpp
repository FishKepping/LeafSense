#include "amg8833_decoder.h"

#include <cstddef>
#include <cstdint>

namespace leafsense {

namespace {

constexpr float PIXEL_RESOLUTION_CELSIUS =
    0.25f;

constexpr float THERMISTOR_RESOLUTION_CELSIUS =
    0.0625f;

constexpr std::uint16_t TWELVE_BIT_MASK =
    0x0FFFU;

constexpr std::uint16_t TWELVE_BIT_SIGN_MASK =
    0x0800U;

constexpr std::uint16_t THERMISTOR_MAGNITUDE_MASK =
    0x07FFU;

constexpr std::int16_t TWELVE_BIT_RANGE =
    0x1000;

}  // namespace

float Amg8833Decoder::decodePixelTemperature(
    std::uint8_t low_byte,
    std::uint8_t high_byte)
{
    const std::uint16_t raw_value =
        combine12BitValue(
            low_byte,
            high_byte);

    const std::int16_t signed_value =
        decodeTwosComplement12(
            raw_value);

    return static_cast<float>(signed_value) *
           PIXEL_RESOLUTION_CELSIUS;
}

float Amg8833Decoder::decodeThermistorTemperature(
    std::uint8_t low_byte,
    std::uint8_t high_byte)
{
    const std::uint16_t raw_value =
        combine12BitValue(
            low_byte,
            high_byte);

    const std::uint16_t magnitude =
        raw_value &
        THERMISTOR_MAGNITUDE_MASK;

    float temperature =
        static_cast<float>(magnitude) *
        THERMISTOR_RESOLUTION_CELSIUS;

    if ((raw_value & TWELVE_BIT_SIGN_MASK) != 0U)
    {
        temperature = -temperature;
    }

    return temperature;
}

ThermalFrame Amg8833Decoder::decodePixels(
    const PixelBytes& pixel_bytes)
{
    ThermalFrame frame;

    for (std::size_t pixel_index = 0;
         pixel_index < ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        const std::size_t byte_index =
            pixel_index * 2;

        const float temperature =
            decodePixelTemperature(
                pixel_bytes[byte_index],
                pixel_bytes[byte_index + 1]);

        const auto x =
            static_cast<std::uint8_t>(
                pixel_index %
                ThermalFrame::WIDTH);

        const auto y =
            static_cast<std::uint8_t>(
                pixel_index /
                ThermalFrame::WIDTH);

        frame.setPixel(
            x,
            y,
            temperature);
    }

    return frame;
}

ThermalFrame Amg8833Decoder::decodeFrame(
    const PixelBytes& pixel_bytes,
    std::uint8_t thermistor_low_byte,
    std::uint8_t thermistor_high_byte,
    std::uint32_t frame_number,
    std::uint32_t timestamp_ms,
    bool valid)
{
    ThermalFrame frame =
        decodePixels(pixel_bytes);

    frame.setThermistorTemperature(
        decodeThermistorTemperature(
            thermistor_low_byte,
            thermistor_high_byte));

    frame.setFrameNumber(
        frame_number);

    frame.setTimestampMs(
        timestamp_ms);

    frame.setValid(
        valid);

    return frame;
}

std::uint16_t Amg8833Decoder::combine12BitValue(
    std::uint8_t low_byte,
    std::uint8_t high_byte)
{
    const std::uint16_t combined =
        static_cast<std::uint16_t>(low_byte) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                high_byte & 0x0FU)
            << 8U);

    return combined &
           TWELVE_BIT_MASK;
}

std::int16_t Amg8833Decoder::decodeTwosComplement12(
    std::uint16_t raw_value)
{
    raw_value &=
        TWELVE_BIT_MASK;

    if ((raw_value & TWELVE_BIT_SIGN_MASK) != 0U)
    {
        return static_cast<std::int16_t>(
            static_cast<std::int32_t>(raw_value) -
            TWELVE_BIT_RANGE);
    }

    return static_cast<std::int16_t>(
        raw_value);
}

}  // namespace leafsense