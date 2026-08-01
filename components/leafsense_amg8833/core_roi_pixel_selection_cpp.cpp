#include "pixel_selection.h"

namespace leafsense::roi {

PixelSelection::PixelSelection()
{
    clear();
}

void PixelSelection::clear()
{
    size_ = 0;
}

bool PixelSelection::contains(std::uint8_t index) const
{
    for (std::size_t i = 0; i < size_; ++i)
    {
        if (pixels_[i] == index)
        {
            return true;
        }
    }

    return false;
}

bool PixelSelection::add(std::uint8_t index)
{
    // Reject invalid indices.
    if (index >= MAX_PIXELS)
    {
        return false;
    }

    // Ignore duplicates.
    if (contains(index))
    {
        return false;
    }

    // Selection already full.
    if (size_ >= MAX_PIXELS)
    {
        return false;
    }

    pixels_[size_] = index;
    ++size_;

    return true;
}

std::size_t PixelSelection::size() const
{
    return size_;
}

bool PixelSelection::empty() const
{
    return size_ == 0;
}

std::uint8_t PixelSelection::operator[](std::size_t i) const
{
    return pixels_[i];
}

} // namespace leafsense::roi