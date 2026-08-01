#pragma once

#include <array>
#include <cstddef>

#include "pixel_selection.h"

namespace leafsense::roi {

/**
 * @brief A two-dimensional point in thermal-image coordinates.
 */
struct Point
{
    float x;
    float y;
};

/**
 * @brief Defines a polygonal region within a thermal frame.
 *
 * PolygonRoi stores geometry only and owns no thermal data.
 *
 * Vertices are stored in insertion order. The final vertex is
 * automatically connected back to the first vertex.
 *
 * Pixel selection is performed using pixel centers:
 *
 *     pixel (0,0) has center (0.5, 0.5)
 *     pixel (7,7) has center (7.5, 7.5)
 *
 * Points lying directly on a polygon boundary are included.
 *
 * The class uses fixed-capacity storage and performs no heap
 * allocation, making it suitable for embedded environments.
 */
class PolygonRoi
{
public:
    static constexpr std::size_t MAX_VERTICES = 16;

    /**
     * Construct an empty polygon.
     */
    PolygonRoi();

    /**
     * Remove every vertex.
     */
    void clear();

    /**
     * Add a vertex to the polygon.
     *
     * @return true if the vertex was inserted.
     */
    bool addVertex(float x, float y);

    /**
     * Add a vertex to the polygon.
     *
     * @return true if the vertex was inserted.
     */
    bool addVertex(const Point& point);

    /**
     * Number of stored vertices.
     */
    std::size_t vertexCount() const;

    /**
     * Maximum supported number of vertices.
     */
    constexpr std::size_t capacity() const
    {
        return MAX_VERTICES;
    }

    /**
     * Returns true when no vertices are stored.
     */
    bool empty() const;

    /**
     * Indexed vertex access.
     */
    const Point& vertex(std::size_t index) const;

    /**
     * Returns true when the polygon has at least three vertices.
     */
    bool isValid() const;

    /**
     * Returns true when the supplied point lies inside or on the
     * boundary of the polygon.
     */
    bool contains(float x, float y) const;

    /**
     * Returns the number of thermal pixels selected by the polygon.
     */
    std::size_t pixelCount() const;

    /**
     * Convert the polygon into a PixelSelection.
     */
    PixelSelection selection() const;

private:
    static bool pointOnSegment(
        const Point& point,
        const Point& start,
        const Point& end);

    std::array<Point, MAX_VERTICES> vertices_;
    std::size_t vertex_count_;
};

}  // namespace leafsense::roi