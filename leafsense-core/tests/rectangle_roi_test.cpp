#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

#include "leafsense/frame_statistics.h"
#include "leafsense/roi/pixel_selection.h"
#include "leafsense/roi/rectangle_roi.h"
#include "leafsense/thermal_frame.h"

namespace {

using Catch::Matchers::WithinAbs;
using leafsense::FrameStatistics;
using leafsense::ThermalFrame;
using leafsense::roi::PixelSelection;
using leafsense::roi::RectangleRoi;

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

}  // namespace

TEST_CASE(
    "RectangleRoi defaults to an empty rectangle",
    "[rectangle_roi]")
{
    const RectangleRoi rectangle;

    REQUIRE(rectangle.x() == 0);
    REQUIRE(rectangle.y() == 0);
    REQUIRE(rectangle.width() == 0);
    REQUIRE(rectangle.height() == 0);

    REQUIRE_FALSE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 0);
    REQUIRE(rectangle.selection().empty());
}

TEST_CASE(
    "RectangleRoi stores constructor geometry",
    "[rectangle_roi]")
{
    const RectangleRoi rectangle(2, 3, 4, 5);

    REQUIRE(rectangle.x() == 2);
    REQUIRE(rectangle.y() == 3);
    REQUIRE(rectangle.width() == 4);
    REQUIRE(rectangle.height() == 5);
}

TEST_CASE(
    "RectangleRoi geometry can be changed",
    "[rectangle_roi]")
{
    RectangleRoi rectangle;

    rectangle.set(1, 2, 3, 4);

    REQUIRE(rectangle.x() == 1);
    REQUIRE(rectangle.y() == 2);
    REQUIRE(rectangle.width() == 3);
    REQUIRE(rectangle.height() == 4);

    rectangle.setX(4);
    rectangle.setY(5);
    rectangle.setWidth(2);
    rectangle.setHeight(3);

    REQUIRE(rectangle.x() == 4);
    REQUIRE(rectangle.y() == 5);
    REQUIRE(rectangle.width() == 2);
    REQUIRE(rectangle.height() == 3);
}

TEST_CASE(
    "RectangleRoi rejects zero width",
    "[rectangle_roi][validation]")
{
    const RectangleRoi rectangle(0, 0, 0, 4);

    REQUIRE_FALSE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 0);
    REQUIRE(rectangle.selection().empty());
}

TEST_CASE(
    "RectangleRoi rejects zero height",
    "[rectangle_roi][validation]")
{
    const RectangleRoi rectangle(0, 0, 4, 0);

    REQUIRE_FALSE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 0);
    REQUIRE(rectangle.selection().empty());
}

TEST_CASE(
    "RectangleRoi rejects an origin outside the frame",
    "[rectangle_roi][validation]")
{
    const RectangleRoi outside_x(8, 0, 1, 1);
    const RectangleRoi outside_y(0, 8, 1, 1);
    const RectangleRoi outside_both(8, 8, 1, 1);

    REQUIRE_FALSE(outside_x.isValid());
    REQUIRE_FALSE(outside_y.isValid());
    REQUIRE_FALSE(outside_both.isValid());

    REQUIRE(outside_x.selection().empty());
    REQUIRE(outside_y.selection().empty());
    REQUIRE(outside_both.selection().empty());
}

TEST_CASE(
    "RectangleRoi creates a single-pixel selection",
    "[rectangle_roi][selection]")
{
    const RectangleRoi rectangle(3, 2, 1, 1);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 1);

    REQUIRE(selection.size() == 1);
    REQUIRE(selection[0] == 19);
}

TEST_CASE(
    "RectangleRoi creates a two-by-two selection",
    "[rectangle_roi][selection]")
{
    const RectangleRoi rectangle(1, 1, 2, 2);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.pixelCount() == 4);
    REQUIRE(selection.size() == 4);

    REQUIRE(selection[0] == 9);
    REQUIRE(selection[1] == 10);
    REQUIRE(selection[2] == 17);
    REQUIRE(selection[3] == 18);
}

TEST_CASE(
    "RectangleRoi produces row-major pixel ordering",
    "[rectangle_roi][selection]")
{
    const RectangleRoi rectangle(2, 3, 3, 2);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(selection.size() == 6);

    REQUIRE(selection[0] == 26);
    REQUIRE(selection[1] == 27);
    REQUIRE(selection[2] == 28);

    REQUIRE(selection[3] == 34);
    REQUIRE(selection[4] == 35);
    REQUIRE(selection[5] == 36);
}

TEST_CASE(
    "RectangleRoi creates a full-frame selection",
    "[rectangle_roi][selection]")
{
    const RectangleRoi rectangle(0, 0, 8, 8);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.isValid());
    REQUIRE(
        rectangle.pixelCount() ==
        ThermalFrame::PIXEL_COUNT);

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
    "RectangleRoi clips width at the right frame boundary",
    "[rectangle_roi][clipping]")
{
    const RectangleRoi rectangle(6, 2, 5, 1);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 2);

    REQUIRE(selection.size() == 2);
    REQUIRE(selection[0] == 22);
    REQUIRE(selection[1] == 23);
}

TEST_CASE(
    "RectangleRoi clips height at the bottom frame boundary",
    "[rectangle_roi][clipping]")
{
    const RectangleRoi rectangle(2, 6, 1, 5);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.isValid());
    REQUIRE(rectangle.pixelCount() == 2);

    REQUIRE(selection.size() == 2);
    REQUIRE(selection[0] == 50);
    REQUIRE(selection[1] == 58);
}

TEST_CASE(
    "RectangleRoi clips width and height at the frame boundaries",
    "[rectangle_roi][clipping]")
{
    const RectangleRoi rectangle(6, 6, 10, 10);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.pixelCount() == 4);
    REQUIRE(selection.size() == 4);

    REQUIRE(selection[0] == 54);
    REQUIRE(selection[1] == 55);
    REQUIRE(selection[2] == 62);
    REQUIRE(selection[3] == 63);
}

TEST_CASE(
    "RectangleRoi supports the bottom-right frame pixel",
    "[rectangle_roi][selection]")
{
    const RectangleRoi rectangle(7, 7, 1, 1);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE(rectangle.pixelCount() == 1);
    REQUIRE(selection.size() == 1);
    REQUIRE(selection[0] == 63);
}

TEST_CASE(
    "RectangleRoi selection works with FrameStatistics",
    "[rectangle_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const RectangleRoi rectangle(1, 1, 2, 2);

    const PixelSelection selection =
        rectangle.selection();

    REQUIRE_THAT(
        FrameStatistics::minimum(frame, selection),
        WithinAbs(9.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::maximum(frame, selection),
        WithinAbs(18.0f, TEST_TOLERANCE));

    REQUIRE_THAT(
        FrameStatistics::average(frame, selection),
        WithinAbs(13.5f, TEST_TOLERANCE));
}

TEST_CASE(
    "A full-frame RectangleRoi matches whole-frame statistics",
    "[rectangle_roi][frame_statistics]")
{
    const ThermalFrame frame =
        makeSequentialFrame();

    const RectangleRoi rectangle(0, 0, 8, 8);

    const PixelSelection selection =
        rectangle.selection();

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

TEST_CASE(
    "RectangleRoi can be reused after geometry changes",
    "[rectangle_roi]")
{
    RectangleRoi rectangle(0, 0, 2, 1);

    PixelSelection selection =
        rectangle.selection();

    REQUIRE(selection.size() == 2);
    REQUIRE(selection[0] == 0);
    REQUIRE(selection[1] == 1);

    rectangle.set(6, 7, 2, 1);

    selection = rectangle.selection();

    REQUIRE(selection.size() == 2);
    REQUIRE(selection[0] == 62);
    REQUIRE(selection[1] == 63);
}