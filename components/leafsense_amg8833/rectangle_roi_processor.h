#pragma once

#include <cstddef>
#include <cstdint>

#include "rectangle_roi.h"
#include "thermal_frame.h"

namespace esphome {
namespace leafsense_amg8833 {

struct RectangleRoiResult
{
    float minimum_temperature = 0.0f;
    float maximum_temperature = 0.0f;
    float average_temperature = 0.0f;
    std::size_t pixel_count = 0U;
    bool configured = false;
    bool available = false;
};

class RectangleRoiProcessor
{
public:
    void configure(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t width,
        std::uint8_t height);

    RectangleRoiResult process(
        const leafsense::ThermalFrame& frame) const;

    bool configured() const;
    const leafsense::roi::RectangleRoi& roi() const;

private:
    leafsense::roi::RectangleRoi roi_;
    bool configured_ = false;
};

}  // namespace leafsense_amg8833
}  // namespace esphome
