#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "leafsense/measurement/measurement_channel_controller.h"

namespace {

using Catch::Approx;
using leafsense::measurement::MeasurementChannelCommandStatus;
using leafsense::measurement::MeasurementChannelController;
using leafsense::measurement::MeasurementChannelManager;
using leafsense::measurement::MeasurementChannelType;
using leafsense::measurement::MeasurementPoint;

}  // namespace

TEST_CASE("rectangle runtime update applies atomically and increments revision")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);

    REQUIRE(controller.setRectangle(0U, {1.0f, 2.0f, 3.0f, 4.0f}) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(manager.channel(0U).type() == MeasurementChannelType::Rectangle);
    REQUIRE(controller.revision(0U) == 1U);

    REQUIRE(controller.setRectangle(0U, {1.0f, 2.0f, 3.0f, 4.0f}) ==
            MeasurementChannelCommandStatus::NoChange);
    REQUIRE(controller.revision(0U) == 1U);
}

TEST_CASE("invalid rectangle leaves existing live channel unchanged")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);
    REQUIRE(controller.setRectangle(0U, {0.0f, 0.0f, 2.0f, 2.0f}) ==
            MeasurementChannelCommandStatus::Success);

    REQUIRE(controller.setRectangle(0U, {0.0f, 0.0f, 0.0f, 2.0f}) ==
            MeasurementChannelCommandStatus::InvalidRectangle);
    REQUIRE(manager.channel(0U).type() == MeasurementChannelType::Rectangle);
    REQUIRE(manager.channel(0U).rectangle().width == Approx(2.0f));
    REQUIRE(controller.revision(0U) == 1U);
}

TEST_CASE("polygon edit does not alter live geometry until commit")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);
    REQUIRE(controller.setRectangle(1U, {0.0f, 0.0f, 2.0f, 2.0f}) ==
            MeasurementChannelCommandStatus::Success);

    REQUIRE(controller.beginPolygon(1U, 4U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(controller.setPolygonPoint(1U, 0U, {2.0f, 2.0f}) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(manager.channel(1U).type() == MeasurementChannelType::Rectangle);
    REQUIRE(controller.revision(1U) == 1U);
}

TEST_CASE("complete polygon transaction replaces the live channel")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);

    REQUIRE(controller.beginPolygon(2U, 4U) ==
            MeasurementChannelCommandStatus::Success);
    const MeasurementPoint points[] = {
        {1.0f, 1.0f}, {5.0f, 1.0f}, {5.0f, 5.0f}, {1.0f, 5.0f}};
    for (std::size_t index = 0U; index < 4U; ++index)
    {
        REQUIRE(controller.setPolygonPoint(2U, index, points[index]) ==
                MeasurementChannelCommandStatus::Success);
    }

    REQUIRE(controller.commitPolygon(2U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(manager.channel(2U).type() == MeasurementChannelType::Polygon);
    REQUIRE(manager.channel(2U).polygon().point_count == 4U);
    REQUIRE_FALSE(controller.polygonEditActive(2U));
    REQUIRE(controller.revision(2U) == 1U);
}

TEST_CASE("incomplete polygon cannot replace a live channel")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);
    REQUIRE(controller.setRectangle(3U, {0.0f, 0.0f, 1.0f, 1.0f}) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(controller.beginPolygon(3U, 3U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(controller.setPolygonPoint(3U, 0U, {0.0f, 0.0f}) ==
            MeasurementChannelCommandStatus::Success);

    REQUIRE(controller.commitPolygon(3U) ==
            MeasurementChannelCommandStatus::PolygonIncomplete);
    REQUIRE(manager.channel(3U).type() == MeasurementChannelType::Rectangle);
    REQUIRE(controller.revision(3U) == 1U);
}

TEST_CASE("polygon edit can be cancelled without changing the channel")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);
    REQUIRE(controller.beginPolygon(4U, 3U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(controller.cancelPolygon(4U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE_FALSE(controller.polygonEditActive(4U));
    REQUIRE(manager.channel(4U).type() == MeasurementChannelType::Disabled);
    REQUIRE(controller.revision(4U) == 0U);
}

TEST_CASE("runtime API validates channel and point arguments")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);

    REQUIRE(controller.disable(6U) ==
            MeasurementChannelCommandStatus::InvalidChannel);
    REQUIRE(controller.beginPolygon(0U, 2U) ==
            MeasurementChannelCommandStatus::InvalidPointCount);
    REQUIRE(controller.beginPolygon(0U, 3U) ==
            MeasurementChannelCommandStatus::Success);
    REQUIRE(controller.setPolygonPoint(0U, 3U, {1.0f, 1.0f}) ==
            MeasurementChannelCommandStatus::InvalidPointIndex);
    REQUIRE(controller.setPolygonPoint(
                0U,
                0U,
                {std::numeric_limits<float>::quiet_NaN(), 1.0f}) ==
            MeasurementChannelCommandStatus::InvalidPoint);
}

TEST_CASE("calibration changes have an independent revision")
{
    MeasurementChannelManager manager;
    MeasurementChannelController controller(manager);

    controller.setCalibrationOffset(-0.7f);
    REQUIRE(controller.calibrationOffset() == Approx(-0.7f));
    REQUIRE(controller.calibrationRevision() == 1U);

    controller.setCalibrationOffset(-0.7f);
    REQUIRE(controller.calibrationRevision() == 1U);
}
