#include "leafsense/rendering/thermal_renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <cassert>

namespace leafsense::rendering {
namespace {

struct PaletteStop
{
    float position;
    RgbColor color;
};

std::uint8_t interpolateChannel(std::uint8_t start, std::uint8_t end, float amount)
{
    const float value = static_cast<float>(start) +
        (static_cast<float>(end) - static_cast<float>(start)) * amount;
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0F, 255.0F)));
}

RgbColor interpolateColor(const RgbColor& start, const RgbColor& end, float amount)
{
    return {
        interpolateChannel(start.red, end.red, amount),
        interpolateChannel(start.green, end.green, amount),
        interpolateChannel(start.blue, end.blue, amount)
    };
}

template <std::size_t StopCount>
RgbColor colourFromStops(const std::array<PaletteStop, StopCount>& stops, float position)
{
    const float clamped = std::clamp(position, 0.0F, 1.0F);
    if (clamped <= stops.front().position) return stops.front().color;
    if (clamped >= stops.back().position) return stops.back().color;

    for (std::size_t index = 1; index < stops.size(); ++index)
    {
        if (clamped <= stops[index].position)
        {
            const PaletteStop& lower = stops[index - 1];
            const PaletteStop& upper = stops[index];
            const float span = upper.position - lower.position;
            const float amount = span > 0.0F ? (clamped - lower.position) / span : 0.0F;
            return interpolateColor(lower.color, upper.color, amount);
        }
    }
    return stops.back().color;
}

} // namespace

const RgbColor& RenderedThermalImage::pixel(
    std::size_t x,
    std::size_t y) const
{
    static constexpr RgbColor invalid_pixel{0U, 0U, 0U};

    if (x >= width ||
        y >= height ||
        pixels.empty())
    {
        return invalid_pixel;
    }

    return pixels[(y * width) + x];
}

RenderedThermalImage ThermalRenderer::render(
    const ThermalFrame& frame,
    const ThermalRendererOptions& options) const
{
    RenderedThermalImage image;
    if (options.output_width == 0 || options.output_height == 0) return image;

    image.width = options.output_width;
    image.height = options.output_height;
    image.pixels.assign(image.width * image.height, options.invalid_pixel_color);

    float minimum = 0.0F;
    float maximum = 0.0F;
    if (!determineScale(frame, options, minimum, maximum))
    {
        image.scale_min_temperature = minimum;
        image.scale_max_temperature = maximum;
        return image;
    }

    image.scale_min_temperature = minimum;
    image.scale_max_temperature = maximum;

    for (std::size_t output_y = 0; output_y < image.height; ++output_y)
    {
        const float source_y = image.height == 1 ? 0.0F :
            static_cast<float>(output_y) * static_cast<float>(ThermalFrame::HEIGHT - 1) /
            static_cast<float>(image.height - 1);

        for (std::size_t output_x = 0; output_x < image.width; ++output_x)
        {
            const float source_x = image.width == 1 ? 0.0F :
                static_cast<float>(output_x) * static_cast<float>(ThermalFrame::WIDTH - 1) /
                static_cast<float>(image.width - 1);

            const float temperature = sampleFrame(
                frame, source_x, source_y, options.interpolation);
            if (!validTemperature(temperature)) continue;

            const float normalised = clamp01((temperature - minimum) / (maximum - minimum));
            image.pixels[(output_y * image.width) + output_x] =
                paletteColor(options.palette, normalised);
        }
    }

    return image;
}

RgbColor ThermalRenderer::paletteColor(ThermalPalette palette, float normalised_temperature)
{
    const float value = clamp01(normalised_temperature);
    switch (palette)
    {
        case ThermalPalette::WhiteHot:
        case ThermalPalette::Greyscale:
        {
            const auto channel = static_cast<std::uint8_t>(std::lround(value * 255.0F));
            return {channel, channel, channel};
        }
        case ThermalPalette::BlackHot:
        {
            const auto channel = static_cast<std::uint8_t>(std::lround((1.0F - value) * 255.0F));
            return {channel, channel, channel};
        }
        case ThermalPalette::BlueToRed:
        {
            static constexpr std::array<PaletteStop, 6> stops{{
                {0.00F, {0, 0, 128}}, {0.20F, {0, 128, 255}},
                {0.40F, {0, 255, 255}}, {0.60F, {0, 255, 0}},
                {0.80F, {255, 255, 0}}, {1.00F, {255, 0, 0}}
            }};
            return colourFromStops(stops, value);
        }
        case ThermalPalette::Iron:
        {
            static constexpr std::array<PaletteStop, 6> stops{{
                {0.00F, {0, 0, 0}}, {0.20F, {45, 0, 75}},
                {0.40F, {120, 20, 90}}, {0.60F, {210, 55, 45}},
                {0.80F, {255, 170, 40}}, {1.00F, {255, 255, 220}}
            }};
            return colourFromStops(stops, value);
        }
        case ThermalPalette::Inferno:
        {
            static constexpr std::array<PaletteStop, 6> stops{{
                {0.00F, {0, 0, 4}}, {0.20F, {52, 15, 91}},
                {0.40F, {120, 28, 109}}, {0.60F, {188, 55, 84}},
                {0.80F, {249, 142, 8}}, {1.00F, {252, 255, 164}}
            }};
            return colourFromStops(stops, value);
        }
        case ThermalPalette::Plasma:
        {
            static constexpr std::array<PaletteStop, 6> stops{{
                {0.00F, {13, 8, 135}}, {0.20F, {84, 3, 160}},
                {0.40F, {139, 10, 165}}, {0.60F, {194, 56, 131}},
                {0.80F, {244, 136, 73}}, {1.00F, {240, 249, 33}}
            }};
            return colourFromStops(stops, value);
        }
        case ThermalPalette::Rainbow:
        {
            static constexpr std::array<PaletteStop, 7> stops{{
                {0.00F, {0, 0, 255}}, {0.17F, {0, 255, 255}},
                {0.33F, {0, 255, 0}}, {0.50F, {255, 255, 0}},
                {0.67F, {255, 128, 0}}, {0.83F, {255, 0, 0}},
                {1.00F, {255, 255, 255}}
            }};
            return colourFromStops(stops, value);
        }
    }
    return {0, 0, 0};
}

bool ThermalRenderer::validTemperature(float temperature)
{
    return std::isfinite(temperature);
}

float ThermalRenderer::clamp01(float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

float ThermalRenderer::sampleNearest(const ThermalFrame& frame, float source_x, float source_y)
{
    const auto x = static_cast<std::uint8_t>(std::lround(source_x));
    const auto y = static_cast<std::uint8_t>(std::lround(source_y));
    if (!frame.inBounds(x, y) || !frame.pixelValid(x, y))
        return std::numeric_limits<float>::quiet_NaN();
    return frame.pixel(x, y);
}

float ThermalRenderer::sampleBilinear(const ThermalFrame& frame, float source_x, float source_y)
{
    const auto x0 = static_cast<std::uint8_t>(std::floor(source_x));
    const auto y0 = static_cast<std::uint8_t>(std::floor(source_y));
    const auto x1 = static_cast<std::uint8_t>(std::min<std::size_t>(x0 + 1U, ThermalFrame::WIDTH - 1));
    const auto y1 = static_cast<std::uint8_t>(std::min<std::size_t>(y0 + 1U, ThermalFrame::HEIGHT - 1));
    const float x_amount = source_x - static_cast<float>(x0);
    const float y_amount = source_y - static_cast<float>(y0);

    struct WeightedSample { std::uint8_t x; std::uint8_t y; float weight; };
    const std::array<WeightedSample, 4> samples{{
        {x0, y0, (1.0F - x_amount) * (1.0F - y_amount)},
        {x1, y0, x_amount * (1.0F - y_amount)},
        {x0, y1, (1.0F - x_amount) * y_amount},
        {x1, y1, x_amount * y_amount}
    }};

    float weighted_total = 0.0F;
    float valid_weight = 0.0F;
    for (const auto& sample : samples)
    {
        if (!frame.pixelValid(sample.x, sample.y)) continue;
        const float temperature = frame.pixel(sample.x, sample.y);
        if (!validTemperature(temperature)) continue;
        weighted_total += temperature * sample.weight;
        valid_weight += sample.weight;
    }
    if (valid_weight <= 0.0F) return std::numeric_limits<float>::quiet_NaN();
    return weighted_total / valid_weight;
}

float ThermalRenderer::sampleFrame(
    const ThermalFrame& frame,
    float source_x,
    float source_y,
    InterpolationMode mode)
{
    return mode == InterpolationMode::NearestNeighbour
        ? sampleNearest(frame, source_x, source_y)
        : sampleBilinear(frame, source_x, source_y);
}

bool ThermalRenderer::determineScale(
    const ThermalFrame& frame,
    const ThermalRendererOptions& options,
    float& minimum,
    float& maximum)
{
    if (options.scale_mode == TemperatureScaleMode::Fixed)
    {
        minimum = options.fixed_min_temperature;
        maximum = options.fixed_max_temperature;
        return validTemperature(minimum) && validTemperature(maximum) && maximum > minimum;
    }

    minimum = std::numeric_limits<float>::infinity();
    maximum = -std::numeric_limits<float>::infinity();
    for (std::size_t y = 0; y < ThermalFrame::HEIGHT; ++y)
    {
        for (std::size_t x = 0; x < ThermalFrame::WIDTH; ++x)
        {
            const auto px = static_cast<std::uint8_t>(x);
            const auto py = static_cast<std::uint8_t>(y);
            if (!frame.pixelValid(px, py)) continue;
            const float temperature = frame.pixel(px, py);
            if (!validTemperature(temperature)) continue;
            minimum = std::min(minimum, temperature);
            maximum = std::max(maximum, temperature);
        }
    }

    if (!validTemperature(minimum) || !validTemperature(maximum))
    {
        minimum = 0.0F;
        maximum = 0.0F;
        return false;
    }
    if (maximum <= minimum)
    {
        minimum -= 0.5F;
        maximum += 0.5F;
    }
    return true;
}

} // namespace leafsense::rendering
