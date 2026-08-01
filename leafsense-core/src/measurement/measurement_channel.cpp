#include "leafsense/measurement/measurement_channel.h"

#include <algorithm>
#include <cmath>

#include "leafsense/thermal_frame.h"

namespace leafsense::measurement {

namespace {

constexpr float kMinimumDimension = 0.0001f;

bool finitePoint(const MeasurementPoint& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

}  // namespace

MeasurementChannel::MeasurementChannel()
    : type_(MeasurementChannelType::Disabled),
      rectangle_(),
      polygon_(),
      pixel_selection_()
{
}

void MeasurementChannel::disable()
{
    type_ = MeasurementChannelType::Disabled;
    rectangle_ = MeasurementRectangle{};
    polygon_ = MeasurementPolygon{};
    pixel_selection_.clear();
}

bool MeasurementChannel::setRectangle(
    const MeasurementRectangle& rectangle)
{
    if (!std::isfinite(rectangle.x) ||
        !std::isfinite(rectangle.y) ||
        !std::isfinite(rectangle.width) ||
        !std::isfinite(rectangle.height) ||
        rectangle.width <= kMinimumDimension ||
        rectangle.height <= kMinimumDimension)
    {
        return false;
    }

    rectangle_ = rectangle;
    polygon_ = MeasurementPolygon{};
    pixel_selection_.clear();
    type_ = MeasurementChannelType::Rectangle;
    return !selection().empty();
}

bool MeasurementChannel::setPolygon(
    const MeasurementPoint* points,
    std::size_t point_count)
{
    if (points == nullptr ||
        point_count < 3U ||
        point_count > MeasurementPolygon::MAX_POINTS)
    {
        return false;
    }

    MeasurementPolygon candidate{};
    candidate.point_count = point_count;

    for (std::size_t index = 0; index < point_count; ++index)
    {
        if (!finitePoint(points[index]))
        {
            return false;
        }
        candidate.points[index] = points[index];
    }

    polygon_ = candidate;
    rectangle_ = MeasurementRectangle{};
    pixel_selection_.clear();
    type_ = MeasurementChannelType::Polygon;

    if (selection().empty())
    {
        disable();
        return false;
    }

    return true;
}

bool MeasurementChannel::setPixelSelection(
    const roi::PixelSelection& selection)
{
    if (selection.empty())
    {
        return false;
    }

    rectangle_ = MeasurementRectangle{};
    polygon_ = MeasurementPolygon{};
    pixel_selection_ = selection;
    type_ = MeasurementChannelType::PixelMask;
    return true;
}

bool MeasurementChannel::enabled() const
{
    return type_ != MeasurementChannelType::Disabled;
}

bool MeasurementChannel::valid() const
{
    return enabled() && !selection().empty();
}

MeasurementChannelType MeasurementChannel::type() const
{
    return type_;
}

const MeasurementRectangle& MeasurementChannel::rectangle() const
{
    return rectangle_;
}

const MeasurementPolygon& MeasurementChannel::polygon() const
{
    return polygon_;
}

roi::PixelSelection MeasurementChannel::selection() const
{
    if (type_ == MeasurementChannelType::PixelMask)
    {
        return pixel_selection_;
    }

    roi::PixelSelection result;

    if (type_ == MeasurementChannelType::Rectangle)
    {
        const float right = rectangle_.x + rectangle_.width;
        const float bottom = rectangle_.y + rectangle_.height;

        for (std::uint8_t y = 0U;
             y < static_cast<std::uint8_t>(ThermalFrame::HEIGHT);
             ++y)
        {
            for (std::uint8_t x = 0U;
                 x < static_cast<std::uint8_t>(ThermalFrame::WIDTH);
                 ++x)
            {
                const float centre_x = static_cast<float>(x) + 0.5f;
                const float centre_y = static_cast<float>(y) + 0.5f;

                if (centre_x >= rectangle_.x &&
                    centre_x < right &&
                    centre_y >= rectangle_.y &&
                    centre_y < bottom)
                {
                    result.add(static_cast<std::uint8_t>(
                        static_cast<std::size_t>(y) * ThermalFrame::WIDTH + x));
                }
            }
        }
    }
    else if (type_ == MeasurementChannelType::Polygon)
    {
        for (std::uint8_t y = 0U;
             y < static_cast<std::uint8_t>(ThermalFrame::HEIGHT);
             ++y)
        {
            for (std::uint8_t x = 0U;
                 x < static_cast<std::uint8_t>(ThermalFrame::WIDTH);
                 ++x)
            {
                const float centre_x = static_cast<float>(x) + 0.5f;
                const float centre_y = static_cast<float>(y) + 0.5f;

                if (pointInsidePolygon(centre_x, centre_y, polygon_))
                {
                    result.add(static_cast<std::uint8_t>(
                        static_cast<std::size_t>(y) * ThermalFrame::WIDTH + x));
                }
            }
        }
    }

    return result;
}

bool MeasurementChannel::pointInsidePolygon(
    float x,
    float y,
    const MeasurementPolygon& polygon)
{
    bool inside = false;
    std::size_t previous = polygon.point_count - 1U;

    for (std::size_t current = 0U;
         current < polygon.point_count;
         ++current)
    {
        const MeasurementPoint& a = polygon.points[current];
        const MeasurementPoint& b = polygon.points[previous];

        const bool crosses =
            ((a.y > y) != (b.y > y)) &&
            (x < (b.x - a.x) * (y - a.y) /
                         ((b.y - a.y) == 0.0f ? 1.0f : (b.y - a.y)) +
                     a.x);

        if (crosses)
        {
            inside = !inside;
        }

        previous = current;
    }

    return inside;
}

}  // namespace leafsense::measurement
