#include "leafsense/roi/rectangle_roi.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "leafsense/thermal_frame.h"

namespace leafsense::roi {

RectangleRoi::RectangleRoi()
    : x_(0),
      y_(0),
      width_(0),
      height_(0)
{
}

RectangleRoi::RectangleRoi(
    std::uint8_t x,
    std::uint8_t y,
    std::uint8_t width,
    std::uint8_t height)
    : x_(x),
      y_(y),
      width_(width),
      height_(height)
{
}

void RectangleRoi::set(
    std::uint8_t x,
    std::uint8_t y,
    std::uint8_t width,
    std::uint8_t height)
{
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

std::uint8_t RectangleRoi::x() const
{
    return x_;
}

std::uint8_t RectangleRoi::y() const
{
    return y_;
}

std::uint8_t RectangleRoi::width() const
{
    return width_;
}

std::uint8_t RectangleRoi::height() const
{
    return height_;
}

void RectangleRoi::setX(std::uint8_t x)
{
    x_ = x;
}

void RectangleRoi::setY(std::uint8_t y)
{
    y_ = y;
}

void RectangleRoi::setWidth(std::uint8_t width)
{
    width_ = width;
}

void RectangleRoi::setHeight(std::uint8_t height)
{
    height_ = height;
}

bool RectangleRoi::isValid() const
{
    if (width_ == 0 || height_ == 0)
    {
        return false;
    }

    if (x_ >= ThermalFrame::WIDTH)
    {
        return false;
    }

    if (y_ >= ThermalFrame::HEIGHT)
    {
        return false;
    }

    return true;
}

std::size_t RectangleRoi::pixelCount() const
{
    if (!isValid())
    {
        return 0;
    }

    const std::size_t start_x =
        static_cast<std::size_t>(x_);

    const std::size_t start_y =
        static_cast<std::size_t>(y_);

    const std::size_t requested_end_x =
        start_x + static_cast<std::size_t>(width_);

    const std::size_t requested_end_y =
        start_y + static_cast<std::size_t>(height_);

    const std::size_t end_x =
        std::min(
            requested_end_x,
            ThermalFrame::WIDTH);

    const std::size_t end_y =
        std::min(
            requested_end_y,
            ThermalFrame::HEIGHT);

    const std::size_t clipped_width =
        end_x - start_x;

    const std::size_t clipped_height =
        end_y - start_y;

    return clipped_width * clipped_height;
}

PixelSelection RectangleRoi::selection() const
{
    PixelSelection result;

    if (!isValid())
    {
        return result;
    }

    const std::size_t start_x =
        static_cast<std::size_t>(x_);

    const std::size_t start_y =
        static_cast<std::size_t>(y_);

    const std::size_t requested_end_x =
        start_x + static_cast<std::size_t>(width_);

    const std::size_t requested_end_y =
        start_y + static_cast<std::size_t>(height_);

    const std::size_t end_x =
        std::min(
            requested_end_x,
            ThermalFrame::WIDTH);

    const std::size_t end_y =
        std::min(
            requested_end_y,
            ThermalFrame::HEIGHT);

    for (std::size_t y = start_y; y < end_y; ++y)
    {
        for (std::size_t x = start_x; x < end_x; ++x)
        {
            const std::size_t index =
                y * ThermalFrame::WIDTH + x;

            result.add(
                static_cast<std::uint8_t>(index));
        }
    }

    return result;
}

}  // namespace leafsense::roi