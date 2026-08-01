#pragma once

#include "thermal_frame_packet.h"

#include <functional>
#include <string>

namespace leafsense::transport {

class ThermalFramePublisher {
public:
    using PublishCallback = std::function<void(const std::string&)>;

    explicit ThermalFramePublisher(PublishCallback callback);

    void set_calibration_revision(std::uint32_t revision);
    [[nodiscard]] std::uint32_t calibration_revision() const;
    [[nodiscard]] std::uint32_t sequence() const;

    [[nodiscard]] std::string make_payload(
        const std::array<float, kThermalPixelCount>& calibrated_temperatures_c,
        std::uint32_t timestamp_ms);

    bool publish(
        const std::array<float, kThermalPixelCount>& calibrated_temperatures_c,
        std::uint32_t timestamp_ms);

private:
    PublishCallback callback_;
    std::uint32_t sequence_{0};
    std::uint32_t calibration_revision_{0};
};

}  // namespace leafsense::transport
