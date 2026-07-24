#pragma once

#include <array>
#include <cstdint>

namespace leafsense {

/**
 * Represents a single thermal frame captured from a thermal sensor.
 *
 * A ThermalFrame is a complete snapshot containing:
 *  - Pixel temperatures
 *  - Thermistor temperature
 *  - Timestamp
 *  - Frame number
 *
 * Future drivers (AMG8833, MLX90640, etc.) will all produce
 * ThermalFrame objects.
 */
class ThermalFrame {
public:
    static constexpr std::size_t WIDTH = 8;
    static constexpr std::size_t HEIGHT = 8;
    static constexpr std::size_t PIXEL_COUNT = WIDTH * HEIGHT;

    ThermalFrame();

    void clear();

    bool isValid() const;
    void setValid(bool valid);

    uint32_t frameNumber() const;
    void setFrameNumber(uint32_t frame);

    uint32_t timestampMs() const;
    void setTimestampMs(uint32_t timestamp);

    float thermistorTemperature() const;
    void setThermistorTemperature(float temperature);

    float pixel(uint8_t x, uint8_t y) const;
    void setPixel(uint8_t x, uint8_t y, float temperature);

    /// Returns true if the coordinates are inside the thermal image.
    bool inBounds(uint8_t x, uint8_t y) const;

private:
    /// Converts an (x,y) coordinate into the array index.
    std::size_t index(uint8_t x, uint8_t y) const;

    std::array<float, PIXEL_COUNT> pixels_;

    float thermistor_temperature_;

    uint32_t frame_number_;

    uint32_t timestamp_ms_;

    bool valid_;
};

} // namespace leafsense