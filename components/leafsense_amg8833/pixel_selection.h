#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace leafsense::roi {

/**
 * @brief Stores a collection of unique pixel indices.
 *
 * PixelSelection is intentionally lightweight.
 *
 * It owns no temperature data.
 *
 * It simply stores references (indices) into a ThermalFrame.
 *
 * Every ROI implementation (rectangle, polygon, threshold mask)
 * produces one of these objects.
 */
class PixelSelection
{
public:

    static constexpr std::size_t MAX_PIXELS = 64;

    PixelSelection();

    /**
     * Remove every selected pixel.
     */
    void clear();

    /**
     * Add a pixel index.
     *
     * Duplicate indices are ignored.
     *
     * @return true if inserted.
     */
    bool add(std::uint8_t index);

    /**
     * Returns true if the index already exists.
     */
    bool contains(std::uint8_t index) const;

    /**
     * Number of selected pixels.
     */
    std::size_t size() const;

    /**
     * Returns true if no pixels are selected.
     */
    bool empty() const;

    /**
     * Maximum possible selection size.
     */
    constexpr std::size_t capacity() const
    {
        return MAX_PIXELS;
    }

    /**
     * Indexed access.
     */
    std::uint8_t operator[](std::size_t i) const;

    /**
     * STL iteration support.
     */
    auto begin() const
    {
        return pixels_.begin();
    }

    auto end() const
    {
        return pixels_.begin() + size_;
    }

private:

    std::array<std::uint8_t, MAX_PIXELS> pixels_;

    std::size_t size_;
};

} // namespace leafsense::roi