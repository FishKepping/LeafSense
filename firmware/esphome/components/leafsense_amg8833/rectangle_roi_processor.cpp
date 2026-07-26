#include "rectangle_roi_processor.h"

#include "leafsense/frame_statistics.h"
#include "leafsense/roi/pixel_selection.h"

namespace esphome {
namespace leafsense_amg8833 {

void RectangleRoiProcessor::configure(
    std::uint8_t x,
    std::uint8_t y,
    std::uint8_t width,
    std::uint8_t height)
{
    roi_.set(x, y, width, height);
    configured_ = roi_.isValid();
}

RectangleRoiResult RectangleRoiProcessor::process(
    const leafsense::ThermalFrame& frame) const
{
    RectangleRoiResult result;
    result.configured = configured_;

    if (!configured_ || !frame.isValid())
    {
        return result;
    }

    const leafsense::roi::PixelSelection selection =
        roi_.selection();

    if (selection.empty())
    {
        return result;
    }

    result.minimum_temperature =
        leafsense::FrameStatistics::minimum(frame, selection);
    result.maximum_temperature =
        leafsense::FrameStatistics::maximum(frame, selection);
    result.average_temperature =
        leafsense::FrameStatistics::average(frame, selection);
    result.pixel_count = selection.size();
    result.available = true;

    return result;
}

bool RectangleRoiProcessor::configured() const
{
    return configured_;
}

const leafsense::roi::RectangleRoi&
RectangleRoiProcessor::roi() const
{
    return roi_;
}

}  // namespace leafsense_amg8833
}  // namespace esphome
