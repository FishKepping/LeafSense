#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace leafsense {

/**
 * @brief Stores a single thermal frame.
 *
 * Design Notes
 * ------------
 * ThermalFrame is a lightweight container representing one complete
 * capture from a thermal sensor.
 *
 * It intentionally contains no processing algorithms.
 * Statistics, filtering and ROI calculations belong in other classes.
 */
class ThermalFrame
{
public:
    static constexpr std::size_t WIDTH = 8;
    static constexpr std::size_t HEIGHT = 8;
    static constexpr std::size_t PIXEL_COUNT = WIDTH * HEIGHT;

    ThermalFrame();

    /// Reset the frame to its default state.
    void clear();

    /// Frame validity.
    bool isValid() const;
    void setValid(bool valid);

    /// Frame sequence number.
    uint32_t frameNumber() const;
    void setFrameNumber(uint32_t frame);

    /// Timestamp in milliseconds.
    uint32_t timestampMs() const;
    void setTimestampMs(uint32_t timestamp);

    /// Sensor thermistor temperature.
    float thermistorTemperature() const;
    void setThermistorTemperature(float temperature);

    /// Read a pixel temperature.
    float pixel(uint8_t x, uint8_t y) const;

    /// Write a pixel temperature.
    void setPixel(uint8_t x, uint8_t y, float temperature);

    /// Returns true if the coordinate is inside the image.
    bool inBounds(uint8_t x, uint8_t y) const;

    /// Read-only access to the pixel buffer.
    const std::array<float, PIXEL_COUNT>& pixels() const;

    /**
     * Returns true if the pixel contains valid data.
     *
     * Future versions will support dead-pixel detection without
     * changing the public API.
     */
    bool pixelValid(uint8_t x, uint8_t y) const;

    /**
     * Counts all valid pixels.
     *
     * Currently this will always return 64.
     * Future sensors may return fewer valid pixels.
     */
    std::size_t validPixelCount() const;

private:
    std::size_t index(uint8_t x, uint8_t y) const;

    std::array<float, PIXEL_COUNT> pixels_;

    float thermistor_temperature_;

    uint32_t frame_number_;

    uint32_t timestamp_ms_;

    bool valid_;
};

} // namespace leafsense