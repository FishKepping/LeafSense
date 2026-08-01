#include "thermal_frame_publisher.h"

#include <utility>

namespace leafsense::transport {

ThermalFramePublisher::ThermalFramePublisher(PublishCallback callback)
    : callback_(std::move(callback)) {}

void ThermalFramePublisher::set_calibration_revision(std::uint32_t revision) {
    calibration_revision_ = revision;
}

std::uint32_t ThermalFramePublisher::calibration_revision() const {
    return calibration_revision_;
}

std::uint32_t ThermalFramePublisher::sequence() const {
    return sequence_;
}

std::string ThermalFramePublisher::make_payload(
    const std::array<float, kThermalPixelCount>& calibrated_temperatures_c,
    std::uint32_t timestamp_ms) {
    ThermalFramePacket packet{};
    packet.metadata.sequence = ++sequence_;
    packet.metadata.timestamp_ms = timestamp_ms;
    packet.metadata.calibration_revision = calibration_revision_;
    packet.temperatures_c = calibrated_temperatures_c;
    return ThermalFramePacketCodec::encode_base64(packet);
}

bool ThermalFramePublisher::publish(
    const std::array<float, kThermalPixelCount>& calibrated_temperatures_c,
    std::uint32_t timestamp_ms) {
    if (!callback_) return false;
    callback_(make_payload(calibrated_temperatures_c, timestamp_ms));
    return true;
}

}  // namespace leafsense::transport
