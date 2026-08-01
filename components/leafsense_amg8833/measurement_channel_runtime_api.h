#pragma once

#include <cstddef>
#include <cstdint>

#include "esphome/components/api/custom_api_device.h"
#include "leafsense/measurement/measurement_channel_controller.h"

namespace esphome::leafsense_amg8833 {

class MeasurementChannelRuntimeApi : public api::CustomAPIDevice
{
public:
    void set_controller(
        leafsense::measurement::MeasurementChannelController* controller);

    void setup();
    void dump_config() const;

private:
    void service_disable_(int channel);
    void service_set_rectangle_(
        int channel,
        float x,
        float y,
        float width,
        float height);
    void service_polygon_begin_(int channel, int point_count);
    void service_polygon_point_(
        int channel,
        int point_index,
        float x,
        float y);
    void service_polygon_commit_(int channel);
    void service_polygon_cancel_(int channel);

    bool convert_channel_(int channel, std::size_t& channel_index) const;
    void log_result_(
        const char* command,
        int channel,
        leafsense::measurement::MeasurementChannelCommandStatus status) const;

    leafsense::measurement::MeasurementChannelController* controller_ = nullptr;
};

}  // namespace esphome::leafsense_amg8833
