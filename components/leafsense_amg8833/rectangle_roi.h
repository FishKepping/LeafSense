#pragma once

#include <cstddef>
#include <cstdint>

#include "pixel_selection.h"

namespace leafsense::roi {

/**
 * @brief Defines a rectangular region within an 8x8 thermal frame.
 *
 * RectangleRoi stores geometry only. It does not own thermal data.
 *
 * Calling selection() converts the rectangle into a PixelSelection
 * containing linear pixel indices.
 *
 * Coordinates begin at the top-left corner:
 *
 *     (0,0) ----------------> x
 *       |
 *       |
 *       |
 *       v
 *       y
 *
 * Rectangles extending beyond the thermal frame are clipped to the
 * valid 8x8 image area.
 */
class RectangleRoi
{
public:
    /**
     * Construct an empty rectangle.
     */
    RectangleRoi();

    /**
     * Construct a rectangle.
     *
     * @param x Left coordinate.
     * @param y Top coordinate.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     */
    RectangleRoi(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t width,
        std::uint8_t height);

    /**
     * Set all rectangle geometry.
     */
    void set(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t width,
        std::uint8_t height);

    /**
     * Left coordinate.
     */
    std::uint8_t x() const;

    /**
     * Top coordinate.
     */
    std::uint8_t y() const;

    /**
     * Rectangle width.
     */
    std::uint8_t width() const;

    /**
     * Rectangle height.
     */
    std::uint8_t height() const;

    /**
     * Change the left coordinate.
     */
    void setX(std::uint8_t x);

    /**
     * Change the top coordinate.
     */
    void setY(std::uint8_t y);

    /**
     * Change the rectangle width.
     */
    void setWidth(std::uint8_t width);

    /**
     * Change the rectangle height.
     */
    void setHeight(std::uint8_t height);

    /**
     * Returns true when the rectangle contains at least one valid
     * thermal-frame pixel.
     */
    bool isValid() const;

    /**
     * Returns the number of pixels produced after frame clipping.
     */
    std::size_t pixelCount() const;

    /**
     * Convert the rectangle into a PixelSelection.
     */
    PixelSelection selection() const;

private:
    std::uint8_t x_;
    std::uint8_t y_;
    std::uint8_t width_;
    std::uint8_t height_;
};

}  // namespace leafsense::roi