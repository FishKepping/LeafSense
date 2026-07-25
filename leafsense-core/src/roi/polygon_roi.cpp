#include "leafsense/roi/polygon_roi.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "leafsense/thermal_frame.h"

namespace leafsense::roi {

namespace {

constexpr float GEOMETRY_EPSILON = 0.00001f;

}  // namespace

PolygonRoi::PolygonRoi()
    : vertices_{},
      vertex_count_(0)
{
}

void PolygonRoi::clear()
{
    vertex_count_ = 0;
}

bool PolygonRoi::addVertex(float x, float y)
{
    return addVertex(Point{x, y});
}

bool PolygonRoi::addVertex(const Point& point)
{
    if (vertex_count_ >= MAX_VERTICES)
    {
        return false;
    }

    vertices_[vertex_count_] = point;
    ++vertex_count_;

    return true;
}

std::size_t PolygonRoi::vertexCount() const
{
    return vertex_count_;
}

bool PolygonRoi::empty() const
{
    return vertex_count_ == 0;
}

const Point& PolygonRoi::vertex(std::size_t index) const
{
    return vertices_[index];
}

bool PolygonRoi::isValid() const
{
    return vertex_count_ >= 3;
}

bool PolygonRoi::pointOnSegment(
    const Point& point,
    const Point& start,
    const Point& end)
{
    const float segment_x = end.x - start.x;
    const float segment_y = end.y - start.y;

    const float point_x = point.x - start.x;
    const float point_y = point.y - start.y;

    const float cross_product =
        segment_x * point_y -
        segment_y * point_x;

    if (std::fabs(cross_product) > GEOMETRY_EPSILON)
    {
        return false;
    }

    const float dot_product =
        point_x * segment_x +
        point_y * segment_y;

    if (dot_product < -GEOMETRY_EPSILON)
    {
        return false;
    }

    const float segment_length_squared =
        segment_x * segment_x +
        segment_y * segment_y;

    if (dot_product >
        segment_length_squared + GEOMETRY_EPSILON)
    {
        return false;
    }

    return true;
}

bool PolygonRoi::contains(float x, float y) const
{
    if (!isValid())
    {
        return false;
    }

    const Point point{x, y};

    bool inside = false;

    std::size_t previous_index =
        vertex_count_ - 1;

    for (std::size_t current_index = 0;
         current_index < vertex_count_;
         ++current_index)
    {
        const Point& current =
            vertices_[current_index];

        const Point& previous =
            vertices_[previous_index];

        if (pointOnSegment(
                point,
                previous,
                current))
        {
            return true;
        }

        const bool crosses_vertical_range =
            (current.y > point.y) !=
            (previous.y > point.y);

        if (crosses_vertical_range)
        {
            const float intersection_x =
                previous.x +
                (point.y - previous.y) *
                    (current.x - previous.x) /
                    (current.y - previous.y);

            if (point.x < intersection_x)
            {
                inside = !inside;
            }
        }

        previous_index = current_index;
    }

    return inside;
}

std::size_t PolygonRoi::pixelCount() const
{
    return selection().size();
}

PixelSelection PolygonRoi::selection() const
{
    PixelSelection result;

    if (!isValid())
    {
        return result;
    }

    for (std::size_t y = 0;
         y < ThermalFrame::HEIGHT;
         ++y)
    {
        for (std::size_t x = 0;
             x < ThermalFrame::WIDTH;
             ++x)
        {
            const float center_x =
                static_cast<float>(x) + 0.5f;

            const float center_y =
                static_cast<float>(y) + 0.5f;

            if (!contains(center_x, center_y))
            {
                continue;
            }

            const std::size_t pixel_index =
                y * ThermalFrame::WIDTH + x;

            result.add(
                static_cast<std::uint8_t>(
                    pixel_index));
        }
    }

    return result;
}

}  // namespace leafsense::roi