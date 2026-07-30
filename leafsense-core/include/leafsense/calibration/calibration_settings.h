#pragma once

#include <cstdint>

namespace leafsense::calibration {

struct CalibrationSettings
{
    static constexpr float DEFAULT_GAIN = 1.0F;
    static constexpr float DEFAULT_OFFSET_C = 0.0F;

    float gain{DEFAULT_GAIN};
    float offset_c{DEFAULT_OFFSET_C};
    std::uint32_t revision{0};

    static CalibrationSettings defaults()
    {
        return {};
    }

    bool operator==(const CalibrationSettings& other) const
    {
        return gain == other.gain &&
               offset_c == other.offset_c &&
               revision == other.revision;
    }

    bool operator!=(const CalibrationSettings& other) const
    {
        return !(*this == other);
    }
};

} // namespace leafsense::calibration
