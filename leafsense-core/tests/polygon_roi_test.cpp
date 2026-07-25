#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/frame_statistics.h"
#include "leafsense/roi/pixel_selection.h"
#include "leafsense/roi/polygon_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::roi::PixelSelection;
using leafsense::roi::Point;
using leafsense::roi::PolygonRoi;

constexpr float TEST_TOLERANCE = 0.0001f;

void setPixelAtIndex(
    ThermalFrame& frame,
    std::size_t index,
    float temperature)
{
    const auto x = static_cast<std::uint8_t>(
        index % ThermalFrame::WIDTH);

    const auto y = static_cast<std::uint8_t>(
        index / ThermalFrame::WIDTH);

    frame.setPixel(x, y, temperature);
}

ThermalFrame makeSequentialFrame()
{
    ThermalFrame frame;

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        setPixelAtIndex(
            frame,
            index,
            static_cast<float>(index));
    }

    return frame;
}

PolygonRoi makeSquare()
{
    PolygonRoi polygon;

    polygon.addVertex(1.0f, 1.0f);
    polygon.addVertex(4.0f, 1.0f);
    polygon.addVertex(4.0f, 4.0f);
    polygon.addVertex(1.0f, 4.0f);

    return polygon;
}

}  // namespace

TEST_CASE(
    "PolygonRoi defaults to an empty polygon",
    "[polygon_roi]")
{
    const PolygonRoi polygon;

    REQUIRE(polygon.empty());
    REQUIRE(polygon.vertexCount() == 0);
    REQUIRE_FALSE(polygon.isValid());
    REQUIRE(polygon.pixelCount() == 0);
    REQUIRE(polygon.selection().empty());
}

TEST_CASE(
    "PolygonRoi stores vertices in insertion order",
    "[polygon_roi]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(1.0f, 2.0f));
    REQUIRE(polygon.addVertex(Point{3.0f, 4.0f}));
    REQUIRE(polygon.addVertex(5.0f, 6.0f));

    REQUIRE(polygon.vertexCount() == 3);

    REQUIRE_THAT(
        polygon.vertex(0).x,
        WithinAbs(1.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        polygon.vertex(0).y,
        WithinAbs(2.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        polygon.vertex(1).x,
        WithinAbs(3.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        polygon.vertex(1).y,
        WithinAbs(4.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        polygon.vertex(2).x,
        WithinAbs(5.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        polygon.vertex(2).y,
        WithinAbs(6.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "PolygonRoi requires at least three vertices",
    "[polygon_roi][validation]")
{
    PolygonRoi polygon;

    REQUIRE_FALSE(polygon.isValid());

    REQUIRE(polygon.addVertex(0.0f, 0.0f));
    REQUIRE_FALSE(polygon.isValid());

    REQUIRE(polygon.addVertex(1.0f, 0.0f));
    REQUIRE_FALSE(polygon.isValid());

    REQUIRE(polygon.addVertex(0.0f, 1.0f));
    REQUIRE(polygon.isValid());
}

TEST_CASE(
    "PolygonRoi supports its fixed vertex capacity",
    "[polygon_roi][capacity]")
{
    PolygonRoi polygon;

    for (std::size_t index = 0;
         index < PolygonRoi::MAX_VERTICES;
         ++index)
    {
        REQUIRE(
            polygon.addVertex(
                static_cast<float>(index),
                static_cast<float>(index)));
    }

    REQUIRE(
        polygon.vertexCount() ==
        PolygonRoi::MAX_VERTICES);

    REQUIRE_FALSE(
        polygon.addVertex(100.0f, 100.0f));
}

TEST_CASE(
    "PolygonRoi can be cleared",
    "[polygon_roi]")
{
    PolygonRoi polygon = makeSquare();

    REQUIRE(polygon.isValid());
    REQUIRE(polygon.vertexCount() == 4);

    polygon.clear();

    REQUIRE(polygon.empty());
    REQUIRE(polygon.vertexCount() == 0);
    REQUIRE_FALSE(polygon.isValid());
    REQUIRE(polygon.selection().empty());
}

TEST_CASE(
    "PolygonRoi detects points inside a square",
    "[polygon_roi][contains]")
{
    const PolygonRoi polygon = makeSquare();

    REQUIRE(polygon.contains(1.5f, 1.5f));
    REQUIRE(polygon.contains(2.5f, 2.5f));
    REQUIRE(polygon.contains(3.5f, 3.5f));
}

TEST_CASE(
    "PolygonRoi rejects points outside a square",
    "[polygon_roi][contains]")
{
    const PolygonRoi polygon = makeSquare();

    REQUIRE_FALSE(polygon.contains(0.5f, 0.5f));
    REQUIRE_FALSE(polygon.contains(4.5f, 2.5f));
    REQUIRE_FALSE(polygon.contains(2.5f, 4.5f));
}

TEST_CASE(
    "PolygonRoi includes points on polygon boundaries",
    "[polygon_roi][contains]")
{
    const PolygonRoi polygon = makeSquare();

    REQUIRE(polygon.contains(1.0f, 2.0f));
    REQUIRE(polygon.contains(4.0f, 2.0f));
    REQUIRE(polygon.contains(2.0f, 1.0f));
    REQUIRE(polygon.contains(2.0f, 4.0f));

    REQUIRE(polygon.contains(1.0f, 1.0f));
    REQUIRE(polygon.contains(4.0f, 4.0f));
}

TEST_CASE(
    "PolygonRoi creates a square pixel selection",
    "[polygon_roi][selection]")
{
    const PolygonRoi polygon = makeSquare();

    const PixelSelection selection =
        polygon.selection();

    REQUIRE(polygon.pixelCount() == 9);
    REQUIRE(selection.size() == 9);

    REQUIRE(selection[0] == 9);
    REQUIRE(selection[1] == 10);
    REQUIRE(selection[2] == 11);

    REQUIRE(selection[3] == 17);
    REQUIRE(selection[4] == 18);
    REQUIRE(selection[5] == 19);

    REQUIRE(selection[6] == 25);
    REQUIRE(selection[7] == 26);
    REQUIRE(selection[8] == 27);
}

TEST_CASE(
    "PolygonRoi creates a triangular pixel selection",
    "[polygon_roi][selection]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(0.0f, 0.0f));
    REQUIRE(polygon.addVertex(4.0f, 0.0f));
    REQUIRE(polygon.addVertex(0.0f, 4.0f));

    const PixelSelection selection =
        polygon.selection();

    REQUIRE(selection.size() == 10);

    REQUIRE(selection[0] == 0);
    REQUIRE(selection[1] == 1);
    REQUIRE(selection[2] == 2);
    REQUIRE(selection[3] == 3);

    REQUIRE(selection[4] == 8);
    REQUIRE(selection[5] == 9);
    REQUIRE(selection[6] == 10);

    REQUIRE(selection[7] == 16);
    REQUIRE(selection[8] == 17);

    REQUIRE(selection[9] == 24);
}

TEST_CASE(
    "PolygonRoi creates a full-frame selection",
    "[polygon_roi][selection]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(0.0f, 0.0f));
    REQUIRE(polygon.addVertex(8.0f, 0.0f));
    REQUIRE(polygon.addVertex(8.0f, 8.0f));
    REQUIRE(polygon.addVertex(0.0f, 8.0f));

    const PixelSelection selection =
        polygon.selection();

    REQUIRE(
        selection.size() ==
        ThermalFrame::PIXEL_COUNT);

    for (std::size_t index = 0;
         index < ThermalFrame::PIXEL_COUNT;
         ++index)
    {
        REQUIRE(
            selection[index] ==
            static_cast<std::uint8_t>(index));
    }
}

TEST_CASE(
    "PolygonRoi clips naturally to the thermal frame",
    "[polygon_roi][selection]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(-10.0f, -10.0f));
    REQUIRE(polygon.addVertex(20.0f, -10.0f));
    REQUIRE(polygon.addVertex(20.0f, 20.0f));
    REQUIRE(polygon.addVertex(-10.0f, 20.0f));

    const PixelSelection selection =
        polygon.selection();

    REQUIRE(
        selection.size() ==
        ThermalFrame::PIXEL_COUNT);
}

TEST_CASE(
    "PolygonRoi outside the frame produces an empty selection",
    "[polygon_roi][selection]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(20.0f, 20.0f));
    REQUIRE(polygon.addVertex(30.0f, 20.0f));
    REQUIRE(polygon.addVertex(30.0f, 30.0f));
    REQUIRE(polygon.addVertex(20.0f, 30.0f));

    REQUIRE(polygon.isValid());
    REQUIRE(polygon.pixelCount() == 0);
    REQUIRE(polygon.selection().empty());
}

TEST_CASE(
    "PolygonRoi supports clockwise vertex order",
    "[polygon_roi][contains]")
{
    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(1.0f, 1.0f));
    REQUIRE(polygon.addVertex(1.0f, 4.0f));
    REQUIRE(polygon.addVertex(4.0f, 4.0f));
    REQUIRE(polygon.addVertex(4.0f, 1.0f));

    REQUIRE(polygon.contains(2.5f, 2.5f));
    REQUIRE(polygon.selection().size() == 9);
}

TEST_CASE(
    "PolygonRoi selection works with FrameStatistics",
    "[polygon_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const PolygonRoi polygon =
        makeSquare();

    const PixelSelection selection =
        polygon.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(9.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(27.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(18.0f, TEST_TOLERANCE));
}

TEST_CASE(
    "A full-frame PolygonRoi matches whole-frame statistics",
    "[polygon_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    PolygonRoi polygon;

    REQUIRE(polygon.addVertex(0.0f, 0.0f));
    REQUIRE(polygon.addVertex(8.0f, 0.0f));
    REQUIRE(polygon.addVertex(8.0f, 8.0f));
    REQUIRE(polygon.addVertex(0.0f, 8.0f));

    const PixelSelection selection =
        polygon.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(
            FrameStatistics::minimum(frame),
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(
            FrameStatistics::maximum(frame),
            TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(
            FrameStatistics::average(frame),
            TEST_TOLERANCE));
}