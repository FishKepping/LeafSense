#include "leafsense/thermal_frame.h"

namespace leafsense {

ThermalFrame::ThermalFrame()
{
    clear();
}

void ThermalFrame::clear()
{
    pixels_.fill(0.0f);

    thermistor_temperature_ = 0.0f;

    frame_number_ = 0;

    timestamp_ms_ = 0;

    valid_ = false;
}

bool ThermalFrame::isValid() const
{
    return valid_;
}

void ThermalFrame::setValid(bool valid)
{
    valid_ = valid;
}

uint32_t ThermalFrame::frameNumber() const
{
    return frame_number_;
}

void ThermalFrame::setFrameNumber(uint32_t frame)
{
    frame_number_ = frame;
}

uint32_t ThermalFrame::timestampMs() const
{
    return timestamp_ms_;
}

void ThermalFrame::setTimestampMs(uint32_t timestamp)
{
    timestamp_ms_ = timestamp;
}

float ThermalFrame::thermistorTemperature() const
{
    return thermistor_temperature_;
}

void ThermalFrame::setThermistorTemperature(float temperature)
{
    thermistor_temperature_ = temperature;
}

bool ThermalFrame::inBounds(uint8_t x, uint8_t y) const
{
    return (x < WIDTH) && (y < HEIGHT);
}

std::size_t ThermalFrame::index(uint8_t x, uint8_t y) const
{
    return static_cast<std::size_t>(y) * WIDTH +
           static_cast<std::size_t>(x);
}

float ThermalFrame::pixel(uint8_t x, uint8_t y) const
{
    if (!inBounds(x, y))
    {
        return 0.0f;
    }

    return pixels_[index(x, y)];
}

void ThermalFrame::setPixel(uint8_t x,
                            uint8_t y,
                            float temperature)
{
    if (!inBounds(x, y))
    {
        return;
    }

    pixels_[index(x, y)] = temperature;
}

const std::array<float, ThermalFrame::PIXEL_COUNT>&
ThermalFrame::pixels() const
{
    return pixels_;
}

bool ThermalFrame::pixelValid(uint8_t x, uint8_t y) const
{
    //
    // Version 0.2A
    //
    // Every in-bounds pixel is considered valid.
    //
    // Future versions will support:
    //
    //  • Dead pixel detection
    //  • Sensor read failures
    //  • Per-pixel quality flags
    //  • Interpolation masks
    //
    return inBounds(x, y);
}

std::size_t ThermalFrame::validPixelCount() const
{
    std::size_t count = 0;

    for (std::size_t y = 0; y < HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < WIDTH; ++x)
        {
            if (pixelValid(
                    static_cast<uint8_t>(x),
                    static_cast<uint8_t>(y)))
            {
                ++count;
            }
        }
    }

    return count;
}

} // namespace leafsense