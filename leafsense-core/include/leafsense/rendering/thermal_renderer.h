#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "leafsense/thermal_frame.h"

namespace leafsense::rendering {

struct RgbColor
{
    std::uint8_t red{0};
    std::uint8_t green{0};
    std::uint8_t blue{0};

    bool operator==(const RgbColor& other) const
    {
        return red == other.red && green == other.green && blue == other.blue;
    }

    bool operator!=(const RgbColor& other) const
    {
        return !(*this == other);
    }
};

enum class ThermalPalette
{
    BlueToRed,
    Iron,
    Inferno,
    Plasma,
    Rainbow,
    WhiteHot,
    BlackHot,
    Greyscale
};

enum class TemperatureScaleMode
{
    Automatic,
    Fixed
};

enum class InterpolationMode
{
    NearestNeighbour,
    Bilinear
};

struct ThermalRendererOptions
{
    std::size_t output_width{ThermalFrame::WIDTH};
    std::size_t output_height{ThermalFrame::HEIGHT};
    ThermalPalette palette{ThermalPalette::BlueToRed};
    TemperatureScaleMode scale_mode{TemperatureScaleMode::Automatic};
    InterpolationMode interpolation{InterpolationMode::Bilinear};
    float fixed_min_temperature{15.0F};
    float fixed_max_temperature{40.0F};
    RgbColor invalid_pixel_color{0, 0, 0};
};

struct RenderedThermalImage
{
    std::size_t width{0};
    std::size_t height{0};
    float scale_min_temperature{0.0F};
    float scale_max_temperature{0.0F};
    std::vector<RgbColor> pixels;

    bool empty() const { return pixels.empty(); }
    const RgbColor& pixel(std::size_t x, std::size_t y) const;
};

class ThermalRenderer
{
public:
    RenderedThermalImage render(
        const ThermalFrame& frame,
        const ThermalRendererOptions& options = {}) const;

    static RgbColor paletteColor(
        ThermalPalette palette,
        float normalised_temperature);

private:
    static bool validTemperature(float temperature);
    static float clamp01(float value);
    static float sampleNearest(const ThermalFrame& frame, float source_x, float source_y);
    static float sampleBilinear(const ThermalFrame& frame, float source_x, float source_y);
    static float sampleFrame(
        const ThermalFrame& frame,
        float source_x,
        float source_y,
        InterpolationMode mode);
    static bool determineScale(
        const ThermalFrame& frame,
        const ThermalRendererOptions& options,
        float& minimum,
        float& maximum);
};

} // namespace leafsense::rendering
