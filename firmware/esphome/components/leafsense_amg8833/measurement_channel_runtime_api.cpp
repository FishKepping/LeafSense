#include "measurement_channel_runtime_api.h"

#include <array>
#include <string>

#include "esphome/core/log.h"

namespace esphome::leafsense_amg8833 {

namespace {

static const char* const TAG = "leafsense.channel_api";
constexpr int kFirstPublicChannel = 1;
constexpr int kLastPublicChannel = 6;

}  // namespace

void MeasurementChannelRuntimeApi::set_controller(
    leafsense::measurement::MeasurementChannelController* controller)
{
    controller_ = controller;
}

void MeasurementChannelRuntimeApi::setup()
{
    register_service(
        &MeasurementChannelRuntimeApi::service_disable_,
        "leafsense_channel_disable",
        std::array<std::string, 1>{"channel"});

    register_service(
        &MeasurementChannelRuntimeApi::service_set_rectangle_,
        "leafsense_channel_set_rectangle",
        std::array<std::string, 5>{
            "channel", "x", "y", "width", "height"});

    register_service(
        &MeasurementChannelRuntimeApi::service_polygon_begin_,
        "leafsense_channel_polygon_begin",
        std::array<std::string, 2>{"channel", "point_count"});

    register_service(
        &MeasurementChannelRuntimeApi::service_polygon_point_,
        "leafsense_channel_polygon_point",
        std::array<std::string, 4>{"channel", "point_index", "x", "y"});

    register_service(
        &MeasurementChannelRuntimeApi::service_polygon_commit_,
        "leafsense_channel_polygon_commit",
        std::array<std::string, 1>{"channel"});

    register_service(
        &MeasurementChannelRuntimeApi::service_polygon_cancel_,
        "leafsense_channel_polygon_cancel",
        std::array<std::string, 1>{"channel"});

    register_service(
        &MeasurementChannelRuntimeApi::service_set_calibration_offset_,
        "leafsense_set_calibration_offset",
        std::array<std::string, 1>{"offset_celsius"});
}

void MeasurementChannelRuntimeApi::dump_config()
{
    ESP_LOGCONFIG(TAG, "LeafSense runtime measurement channel API:");
    ESP_LOGCONFIG(TAG, "  Controller connected: %s", controller_ != nullptr ? "yes" : "no");
    ESP_LOGCONFIG(TAG, "  Public channels: %d-%d", kFirstPublicChannel, kLastPublicChannel);
}

float MeasurementChannelRuntimeApi::get_setup_priority() const
{
    return setup_priority::AFTER_CONNECTION;
}

void MeasurementChannelRuntimeApi::service_disable_(int channel)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("disable", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }
    log_result_("disable", channel, controller_->disable(index));
}

void MeasurementChannelRuntimeApi::service_set_rectangle_(
    int channel,
    float x,
    float y,
    float width,
    float height)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("set_rectangle", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }

    const leafsense::measurement::MeasurementRectangle rectangle{
        x, y, width, height};
    log_result_(
        "set_rectangle",
        channel,
        controller_->setRectangle(index, rectangle));
}

void MeasurementChannelRuntimeApi::service_polygon_begin_(
    int channel,
    int point_count)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("polygon_begin", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }

    const auto status = point_count < 0
                            ? leafsense::measurement::MeasurementChannelCommandStatus::InvalidPointCount
                            : controller_->beginPolygon(index, static_cast<std::size_t>(point_count));
    log_result_("polygon_begin", channel, status);
}

void MeasurementChannelRuntimeApi::service_polygon_point_(
    int channel,
    int point_index,
    float x,
    float y)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("polygon_point", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }

    const auto status = point_index < 0
                            ? leafsense::measurement::MeasurementChannelCommandStatus::InvalidPointIndex
                            : controller_->setPolygonPoint(
                                  index,
                                  static_cast<std::size_t>(point_index),
                                  {x, y});
    log_result_("polygon_point", channel, status);
}

void MeasurementChannelRuntimeApi::service_polygon_commit_(int channel)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("polygon_commit", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }
    log_result_("polygon_commit", channel, controller_->commitPolygon(index));
}

void MeasurementChannelRuntimeApi::service_polygon_cancel_(int channel)
{
    std::size_t index = 0U;
    if (!convert_channel_(channel, index))
    {
        log_result_("polygon_cancel", channel,
                    leafsense::measurement::MeasurementChannelCommandStatus::InvalidChannel);
        return;
    }
    log_result_("polygon_cancel", channel, controller_->cancelPolygon(index));
}

void MeasurementChannelRuntimeApi::service_set_calibration_offset_(
    float offset_celsius)
{
    if (controller_ == nullptr)
    {
        ESP_LOGE(TAG, "Calibration update rejected: controller is not connected");
        return;
    }
    controller_->setCalibrationOffset(offset_celsius);
    ESP_LOGI(TAG, "Calibration offset set to %.3f C (revision %u)",
             offset_celsius,
             static_cast<unsigned>(controller_->calibrationRevision()));
}

bool MeasurementChannelRuntimeApi::convert_channel_(
    int channel,
    std::size_t& channel_index) const
{
    if (controller_ == nullptr)
    {
        ESP_LOGE(TAG, "Runtime command rejected: controller is not connected");
        return false;
    }

    if (channel < kFirstPublicChannel || channel > kLastPublicChannel)
    {
        return false;
    }

    channel_index = static_cast<std::size_t>(channel - kFirstPublicChannel);
    return true;
}

void MeasurementChannelRuntimeApi::log_result_(
    const char* command,
    int channel,
    leafsense::measurement::MeasurementChannelCommandStatus status) const
{
    const char* status_text = leafsense::measurement::toString(status);
    if (status == leafsense::measurement::MeasurementChannelCommandStatus::Success ||
        status == leafsense::measurement::MeasurementChannelCommandStatus::NoChange)
    {
        ESP_LOGI(TAG, "%s channel %d: %s", command, channel, status_text);
        return;
    }

    ESP_LOGW(TAG, "%s channel %d rejected: %s", command, channel, status_text);
}

}  // namespace esphome::leafsense_amg8833
