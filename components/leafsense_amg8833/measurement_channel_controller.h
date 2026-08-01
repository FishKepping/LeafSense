#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "measurement_channel_manager.h"

namespace leafsense::measurement {

enum class MeasurementChannelCommandStatus : std::uint8_t
{
    Success = 0,
    NoChange,
    InvalidChannel,
    InvalidRectangle,
    InvalidPointCount,
    InvalidPointIndex,
    InvalidPoint,
    NoPolygonEdit,
    PolygonIncomplete,
    InvalidPolygon,
    InvalidPixelMask
};

struct MeasurementChannelState
{
    MeasurementChannelType type = MeasurementChannelType::Disabled;
    MeasurementRectangle rectangle{};
    MeasurementPolygon polygon{};
    roi::PixelSelection selection{};
    std::uint32_t revision = 0U;
};

class MeasurementChannelController
{
public:
    static constexpr std::size_t CHANNEL_COUNT =
        MeasurementChannelManager::CHANNEL_COUNT;

    explicit MeasurementChannelController(MeasurementChannelManager& manager);

    MeasurementChannelCommandStatus disable(std::size_t channel_index);

    MeasurementChannelCommandStatus setRectangle(
        std::size_t channel_index,
        const MeasurementRectangle& rectangle);
    MeasurementChannelCommandStatus setPixelSelection(
        std::size_t channel_index,
        const roi::PixelSelection& selection);

    MeasurementChannelCommandStatus beginPolygon(
        std::size_t channel_index,
        std::size_t point_count);

    MeasurementChannelCommandStatus setPolygonPoint(
        std::size_t channel_index,
        std::size_t point_index,
        const MeasurementPoint& point);

    MeasurementChannelCommandStatus commitPolygon(
        std::size_t channel_index);

    MeasurementChannelCommandStatus cancelPolygon(
        std::size_t channel_index);

    bool polygonEditActive(std::size_t channel_index) const;
    std::size_t polygonEditPointCount(std::size_t channel_index) const;

    MeasurementChannelState state(std::size_t channel_index) const;
    std::uint32_t revision(std::size_t channel_index) const;

    void setCalibrationOffset(float offset_celsius);
    float calibrationOffset() const;
    std::uint32_t calibrationRevision() const;

private:
    struct PolygonDraft
    {
        bool active = false;
        std::size_t point_count = 0U;
        std::array<MeasurementPoint, MeasurementPolygon::MAX_POINTS> points{};
        std::array<bool, MeasurementPolygon::MAX_POINTS> assigned{};
    };

    static bool finitePoint(const MeasurementPoint& point);
    static bool rectangleEqual(
        const MeasurementRectangle& lhs,
        const MeasurementRectangle& rhs);
    static bool polygonEqual(
        const MeasurementPolygon& lhs,
        const MeasurementPolygon& rhs);

    bool validChannel(std::size_t channel_index) const;
    void clearDraft(std::size_t channel_index);
    void incrementRevision(std::size_t channel_index);

    MeasurementChannelManager& manager_;
    std::array<PolygonDraft, CHANNEL_COUNT> drafts_{};
    std::array<std::uint32_t, CHANNEL_COUNT> revisions_{};
    std::uint32_t calibration_revision_ = 0U;
};

const char* toString(MeasurementChannelCommandStatus status);

}  // namespace leafsense::measurement
