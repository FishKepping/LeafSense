#include "measurement_channel_controller.h"

#include <cmath>

namespace leafsense::measurement {

MeasurementChannelController::MeasurementChannelController(
    MeasurementChannelManager& manager)
    : manager_(manager)
{
}

MeasurementChannelCommandStatus MeasurementChannelController::disable(
    std::size_t channel_index)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    MeasurementChannel& channel = manager_.channel(channel_index);
    const bool changed = channel.enabled();
    channel.disable();
    clearDraft(channel_index);

    if (!changed)
    {
        return MeasurementChannelCommandStatus::NoChange;
    }

    incrementRevision(channel_index);
    return MeasurementChannelCommandStatus::Success;
}

MeasurementChannelCommandStatus MeasurementChannelController::setRectangle(
    std::size_t channel_index,
    const MeasurementRectangle& rectangle)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    MeasurementChannel candidate;
    if (!candidate.setRectangle(rectangle))
    {
        return MeasurementChannelCommandStatus::InvalidRectangle;
    }

    const MeasurementChannel& existing = manager_.channel(channel_index);
    if (existing.type() == MeasurementChannelType::Rectangle &&
        rectangleEqual(existing.rectangle(), rectangle))
    {
        clearDraft(channel_index);
        return MeasurementChannelCommandStatus::NoChange;
    }

    manager_.channel(channel_index) = candidate;
    clearDraft(channel_index);
    incrementRevision(channel_index);
    return MeasurementChannelCommandStatus::Success;
}

MeasurementChannelCommandStatus MeasurementChannelController::beginPolygon(
    std::size_t channel_index,
    std::size_t point_count)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    if (point_count < 3U || point_count > MeasurementPolygon::MAX_POINTS)
    {
        return MeasurementChannelCommandStatus::InvalidPointCount;
    }

    PolygonDraft& draft = drafts_[channel_index];
    draft = PolygonDraft{};
    draft.active = true;
    draft.point_count = point_count;
    return MeasurementChannelCommandStatus::Success;
}

MeasurementChannelCommandStatus MeasurementChannelController::setPolygonPoint(
    std::size_t channel_index,
    std::size_t point_index,
    const MeasurementPoint& point)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    PolygonDraft& draft = drafts_[channel_index];
    if (!draft.active)
    {
        return MeasurementChannelCommandStatus::NoPolygonEdit;
    }

    if (point_index >= draft.point_count)
    {
        return MeasurementChannelCommandStatus::InvalidPointIndex;
    }

    if (!finitePoint(point))
    {
        return MeasurementChannelCommandStatus::InvalidPoint;
    }

    draft.points[point_index] = point;
    draft.assigned[point_index] = true;
    return MeasurementChannelCommandStatus::Success;
}

MeasurementChannelCommandStatus MeasurementChannelController::commitPolygon(
    std::size_t channel_index)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    PolygonDraft& draft = drafts_[channel_index];
    if (!draft.active)
    {
        return MeasurementChannelCommandStatus::NoPolygonEdit;
    }

    for (std::size_t index = 0U; index < draft.point_count; ++index)
    {
        if (!draft.assigned[index])
        {
            return MeasurementChannelCommandStatus::PolygonIncomplete;
        }
    }

    MeasurementChannel candidate;
    if (!candidate.setPolygon(draft.points.data(), draft.point_count))
    {
        return MeasurementChannelCommandStatus::InvalidPolygon;
    }

    const MeasurementChannel& existing = manager_.channel(channel_index);
    if (existing.type() == MeasurementChannelType::Polygon &&
        polygonEqual(existing.polygon(), candidate.polygon()))
    {
        clearDraft(channel_index);
        return MeasurementChannelCommandStatus::NoChange;
    }

    manager_.channel(channel_index) = candidate;
    clearDraft(channel_index);
    incrementRevision(channel_index);
    return MeasurementChannelCommandStatus::Success;
}

MeasurementChannelCommandStatus MeasurementChannelController::cancelPolygon(
    std::size_t channel_index)
{
    if (!validChannel(channel_index))
    {
        return MeasurementChannelCommandStatus::InvalidChannel;
    }

    if (!drafts_[channel_index].active)
    {
        return MeasurementChannelCommandStatus::NoPolygonEdit;
    }

    clearDraft(channel_index);
    return MeasurementChannelCommandStatus::Success;
}

bool MeasurementChannelController::polygonEditActive(
    std::size_t channel_index) const
{
    return validChannel(channel_index) && drafts_[channel_index].active;
}

std::size_t MeasurementChannelController::polygonEditPointCount(
    std::size_t channel_index) const
{
    if (!validChannel(channel_index) || !drafts_[channel_index].active)
    {
        return 0U;
    }
    return drafts_[channel_index].point_count;
}

MeasurementChannelState MeasurementChannelController::state(
    std::size_t channel_index) const
{
    MeasurementChannelState result{};
    if (!validChannel(channel_index))
    {
        return result;
    }

    const MeasurementChannel& channel = manager_.channel(channel_index);
    result.type = channel.type();
    result.rectangle = channel.rectangle();
    result.polygon = channel.polygon();
    result.revision = revisions_[channel_index];
    return result;
}

std::uint32_t MeasurementChannelController::revision(
    std::size_t channel_index) const
{
    return validChannel(channel_index) ? revisions_[channel_index] : 0U;
}

void MeasurementChannelController::setCalibrationOffset(float offset_celsius)
{
    const float before = manager_.calibrationOffset();
    manager_.setCalibrationOffset(offset_celsius);
    if (manager_.calibrationOffset() != before)
    {
        ++calibration_revision_;
    }
}

float MeasurementChannelController::calibrationOffset() const
{
    return manager_.calibrationOffset();
}

std::uint32_t MeasurementChannelController::calibrationRevision() const
{
    return calibration_revision_;
}

bool MeasurementChannelController::finitePoint(const MeasurementPoint& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool MeasurementChannelController::rectangleEqual(
    const MeasurementRectangle& lhs,
    const MeasurementRectangle& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y &&
           lhs.width == rhs.width && lhs.height == rhs.height;
}

bool MeasurementChannelController::polygonEqual(
    const MeasurementPolygon& lhs,
    const MeasurementPolygon& rhs)
{
    if (lhs.point_count != rhs.point_count)
    {
        return false;
    }

    for (std::size_t index = 0U; index < lhs.point_count; ++index)
    {
        if (lhs.points[index].x != rhs.points[index].x ||
            lhs.points[index].y != rhs.points[index].y)
        {
            return false;
        }
    }
    return true;
}

bool MeasurementChannelController::validChannel(
    std::size_t channel_index) const
{
    return channel_index < CHANNEL_COUNT;
}

void MeasurementChannelController::clearDraft(std::size_t channel_index)
{
    drafts_[channel_index] = PolygonDraft{};
}

void MeasurementChannelController::incrementRevision(
    std::size_t channel_index)
{
    ++revisions_[channel_index];
}

const char* toString(MeasurementChannelCommandStatus status)
{
    switch (status)
    {
        case MeasurementChannelCommandStatus::Success:
            return "success";
        case MeasurementChannelCommandStatus::NoChange:
            return "no_change";
        case MeasurementChannelCommandStatus::InvalidChannel:
            return "invalid_channel";
        case MeasurementChannelCommandStatus::InvalidRectangle:
            return "invalid_rectangle";
        case MeasurementChannelCommandStatus::InvalidPointCount:
            return "invalid_point_count";
        case MeasurementChannelCommandStatus::InvalidPointIndex:
            return "invalid_point_index";
        case MeasurementChannelCommandStatus::InvalidPoint:
            return "invalid_point";
        case MeasurementChannelCommandStatus::NoPolygonEdit:
            return "no_polygon_edit";
        case MeasurementChannelCommandStatus::PolygonIncomplete:
            return "polygon_incomplete";
        case MeasurementChannelCommandStatus::InvalidPolygon:
            return "invalid_polygon";
    }
    return "unknown";
}

}  // namespace leafsense::measurement
