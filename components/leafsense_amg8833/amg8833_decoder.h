#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "thermal_frame.h"

namespace leafsense {

/**
 * @brief Converts raw AMG8833 register data into LeafSense values.
 *
 * This class performs register-data decoding only. It does not perform
 * I2C communication and does not depend on a particular embedded
 * platform.
 *
 * The AMG8833 provides:
 *
 *     64 pixel temperatures
 *     2 bytes per pixel
 *     128 pixel-data bytes in total
 *
 * Pixel temperatures use signed 12-bit two's-complement encoding with
 * a resolution of 0.25 degrees Celsius per count.
 *
 * The internal thermistor uses signed 12-bit sign-and-magnitude
 * encoding with a resolution of 0.0625 degrees Celsius per count.
 *
 * Raw register bytes are supplied in sensor register order:
 *
 *     low byte first
 *     high byte second
 *
 * Pixel bytes are decoded in row-major order:
 *
 *     bytes 0 and 1     -> pixel index 0  -> coordinate (0, 0)
 *     bytes 2 and 3     -> pixel index 1  -> coordinate (1, 0)
 *     ...
 *     bytes 126 and 127 -> pixel index 63 -> coordinate (7, 7)
 *
 * The class is stateless and performs no heap allocation.
 */
class Amg8833Decoder
{
public:
    static constexpr std::size_t PIXEL_BYTE_COUNT =
        ThermalFrame::PIXEL_COUNT * 2;

    using PixelBytes =
        std::array<std::uint8_t, PIXEL_BYTE_COUNT>;

    /**
     * @brief Decode a single pixel-temperature register pair.
     *
     * Pixel temperatures use signed 12-bit two's-complement encoding
     * with a resolution of 0.25 degrees Celsius per count.
     *
     * Bits 7 through 4 of the high byte are ignored.
     */
    static float decodePixelTemperature(
        std::uint8_t low_byte,
        std::uint8_t high_byte);

    /**
     * @brief Decode the sensor thermistor register pair.
     *
     * Thermistor temperatures use signed 12-bit sign-and-magnitude
     * encoding with a resolution of 0.0625 degrees Celsius per count.
     *
     * Bits 7 through 4 of the high byte are ignored.
     */
    static float decodeThermistorTemperature(
        std::uint8_t low_byte,
        std::uint8_t high_byte);

    /**
     * @brief Decode all 64 pixel-temperature register pairs.
     *
     * The returned frame contains decoded pixel temperatures. All other
     * frame metadata retains ThermalFrame's default values.
     */
    static ThermalFrame decodePixels(
        const PixelBytes& pixel_bytes);

    /**
     * @brief Decode a complete sensor sample into a ThermalFrame.
     *
     * @param pixel_bytes Raw bytes from temperature registers
     *                    0x80 through 0xFF.
     * @param thermistor_low_byte Raw register 0x0E.
     * @param thermistor_high_byte Raw register 0x0F.
     * @param frame_number Application-defined frame sequence number.
     * @param timestamp_ms Application-defined acquisition timestamp.
     * @param valid Whether the sensor acquisition was valid.
     */
    static ThermalFrame decodeFrame(
        const PixelBytes& pixel_bytes,
        std::uint8_t thermistor_low_byte,
        std::uint8_t thermistor_high_byte,
        std::uint32_t frame_number,
        std::uint32_t timestamp_ms,
        bool valid = true);

private:
    static std::uint16_t combine12BitValue(
        std::uint8_t low_byte,
        std::uint8_t high_byte);

    static std::int16_t decodeTwosComplement12(
        std::uint16_t raw_value);
};

}  // namespace leafsense