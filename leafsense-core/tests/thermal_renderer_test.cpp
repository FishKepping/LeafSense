#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "leafsense/rendering/thermal_renderer.h"
#include "leafsense/thermal_frame.h"

namespace {
leafsense::ThermalFrame makeGradientFrame()
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);
    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y)
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x)
            frame.setPixel(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y),
                static_cast<float>((y * leafsense::ThermalFrame::WIDTH) + x));
    return frame;
}
}

TEST_CASE("Thermal renderer produces an 8 by 8 image")
{
    const auto frame = makeGradientFrame();
    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.palette = leafsense::rendering::ThermalPalette::WhiteHot;
    const auto image = renderer.render(frame, options);
    REQUIRE(image.width == 8);
    REQUIRE(image.height == 8);
    REQUIRE(image.pixels.size() == 64);
    REQUIRE(image.scale_min_temperature == Catch::Approx(0.0F));
    REQUIRE(image.scale_max_temperature == Catch::Approx(63.0F));
    REQUIRE(image.pixel(0, 0) == leafsense::rendering::RgbColor{0, 0, 0});
    REQUIRE(image.pixel(7, 7) == leafsense::rendering::RgbColor{255, 255, 255});
}

TEST_CASE("Thermal renderer supports a fixed temperature scale")
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);
    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y)
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x)
            frame.setPixel(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 25.0F);

    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.palette = leafsense::rendering::ThermalPalette::WhiteHot;
    options.scale_mode = leafsense::rendering::TemperatureScaleMode::Fixed;
    options.fixed_min_temperature = 20.0F;
    options.fixed_max_temperature = 30.0F;
    const auto image = renderer.render(frame, options);
    REQUIRE(image.scale_min_temperature == Catch::Approx(20.0F));
    REQUIRE(image.scale_max_temperature == Catch::Approx(30.0F));
    REQUIRE(image.pixel(0, 0) == leafsense::rendering::RgbColor{128, 128, 128});
}

TEST_CASE("Thermal renderer supports 16 by 16 bilinear interpolation")
{
    const auto frame = makeGradientFrame();
    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.output_width = 16;
    options.output_height = 16;
    options.palette = leafsense::rendering::ThermalPalette::WhiteHot;
    options.interpolation = leafsense::rendering::InterpolationMode::Bilinear;
    const auto image = renderer.render(frame, options);
    REQUIRE(image.width == 16);
    REQUIRE(image.height == 16);
    REQUIRE(image.pixels.size() == 256);
    REQUIRE(image.pixel(0, 0) == leafsense::rendering::RgbColor{0, 0, 0});
    REQUIRE(image.pixel(15, 15) == leafsense::rendering::RgbColor{255, 255, 255});
}

TEST_CASE("Thermal renderer supports 32 by 32 output")
{
    const auto frame = makeGradientFrame();
    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.output_width = 32;
    options.output_height = 32;
    const auto image = renderer.render(frame, options);
    REQUIRE(image.width == 32);
    REQUIRE(image.height == 32);
    REQUIRE(image.pixels.size() == 1024);
}

TEST_CASE("All renderer palettes map cold and hot values")
{
    using leafsense::rendering::ThermalPalette;
    using leafsense::rendering::ThermalRenderer;
    const ThermalPalette palettes[] = {
        ThermalPalette::BlueToRed, ThermalPalette::Iron, ThermalPalette::Inferno,
        ThermalPalette::Plasma, ThermalPalette::Rainbow, ThermalPalette::WhiteHot,
        ThermalPalette::BlackHot, ThermalPalette::Greyscale
    };
    for (const auto palette : palettes)
        REQUIRE(ThermalRenderer::paletteColor(palette, 0.0F) !=
                ThermalRenderer::paletteColor(palette, 1.0F));
}

TEST_CASE("Renderer clamps temperatures outside a fixed scale")
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);
    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y)
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x)
            frame.setPixel(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), x == 0 ? -100.0F : 100.0F);

    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.palette = leafsense::rendering::ThermalPalette::WhiteHot;
    options.scale_mode = leafsense::rendering::TemperatureScaleMode::Fixed;
    options.fixed_min_temperature = 0.0F;
    options.fixed_max_temperature = 50.0F;
    options.interpolation = leafsense::rendering::InterpolationMode::NearestNeighbour;
    const auto image = renderer.render(frame, options);
    REQUIRE(image.pixel(0, 0) == leafsense::rendering::RgbColor{0, 0, 0});
    REQUIRE(image.pixel(7, 0) == leafsense::rendering::RgbColor{255, 255, 255});
}

TEST_CASE("Renderer uses invalid pixel colour for NaN values")
{
    auto frame = makeGradientFrame();
    frame.setPixel(0, 0, std::numeric_limits<float>::quiet_NaN());
    leafsense::rendering::ThermalRenderer renderer;
    leafsense::rendering::ThermalRendererOptions options;
    options.interpolation = leafsense::rendering::InterpolationMode::NearestNeighbour;
    options.invalid_pixel_color = {12, 34, 56};
    const auto image = renderer.render(frame, options);
    REQUIRE(image.pixel(0, 0) == leafsense::rendering::RgbColor{12, 34, 56});
}

TEST_CASE("Renderer handles a uniform frame")
{
    leafsense::ThermalFrame frame;
    frame.setValid(true);
    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y)
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x)
            frame.setPixel(static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 24.0F);
    leafsense::rendering::ThermalRenderer renderer;
    const auto image = renderer.render(frame);
    REQUIRE(image.scale_min_temperature == Catch::Approx(23.5F));
    REQUIRE(image.scale_max_temperature == Catch::Approx(24.5F));
    REQUIRE(image.pixels.size() == 64);
}

TEST_CASE("Rendered image safely handles an out of range coordinate")
{
    leafsense::rendering::RenderedThermalImage image;

    image.width = 2U;
    image.height = 2U;
    image.pixels = {
        {10U, 20U, 30U},
        {40U, 50U, 60U},
        {70U, 80U, 90U},
        {100U, 110U, 120U}
    };

    const auto& invalid = image.pixel(2U, 0U);

    REQUIRE(invalid.red == 0U);
    REQUIRE(invalid.green == 0U);
    REQUIRE(invalid.blue == 0U);
}
