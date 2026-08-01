#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "leafsense/roi/pixel_selection.h"

namespace leafsense::measurement {

enum class MeasurementChannelType : std::uint8_t
{
    Disabled = 0,
    Rectangle = 1,
    Polygon = 2,
    PixelMask = 3
};

struct MeasurementPoint
{
    float x = 0.0f;
    float y = 0.0f;
};

struct MeasurementRectangle
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct MeasurementPolygon
{
    static constexpr std::size_t MAX_POINTS = 12;

    std::array<MeasurementPoint, MAX_POINTS> points{};
    std::size_t point_count = 0;
};

class MeasurementChannel
{
public:
    MeasurementChannel();

    void disable();
    bool setRectangle(const MeasurementRectangle& rectangle);
    bool setPolygon(const MeasurementPoint* points, std::size_t point_count);
    bool setPixelSelection(const roi::PixelSelection& selection);

    bool enabled() const;
    bool valid() const;
    MeasurementChannelType type() const;

    const MeasurementRectangle& rectangle() const;
    const MeasurementPolygon& polygon() const;

    roi::PixelSelection selection() const;

private:
    static bool pointInsidePolygon(
        float x,
        float y,
        const MeasurementPolygon& polygon);

    MeasurementChannelType type_;
    MeasurementRectangle rectangle_;
    MeasurementPolygon polygon_;
    roi::PixelSelection pixel_selection_;
};

}  // namespace leafsense::measurement
