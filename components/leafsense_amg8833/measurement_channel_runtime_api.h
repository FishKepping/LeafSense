#pragma once

#include <cstddef>
#include <cstdint>

#include "esphome/components/api/custom_api_device.h"
#include "measurement_channel_controller.h"

namespace esphome::leafsense_amg8833 {

class MeasurementChannelRuntimeApi : public api::CustomAPIDevice
{
public:
    void set_controller(
        leafsense::measurement::MeasurementChannelController* controller);

    void setup();
    void dump_config() const;

private:
    void service_disable_(
        std::int32_t channel);

    void service_set_rectangle_(
        std::int32_t channel,
        float x,
        float y,
        float width,
        float height);

    void service_polygon_begin_(
        std::int32_t channel,
        std::int32_t point_count);

    void service_polygon_point_(
        std::int32_t channel,
        std::int32_t point_index,
        float x,
        float y);

    void service_polygon_commit_(
        std::int32_t channel);

    void service_polygon_cancel_(
        std::int32_t channel);

    bool convert_channel_(
        std::int32_t channel,
        std::size_t& channel_index) const;

    void log_result_(
        const char* command,
        std::int32_t channel,
        leafsense::measurement::MeasurementChannelCommandStatus status) const;

    leafsense::measurement::MeasurementChannelController* controller_ =
        nullptr;
};

}  // namespace esphome::leafsense_amg8833