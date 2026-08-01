#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace leafsense::transport {

constexpr std::size_t kThermalFrameWidth = 8;
constexpr std::size_t kThermalFrameHeight = 8;
constexpr std::size_t kThermalPixelCount = kThermalFrameWidth * kThermalFrameHeight;
constexpr std::int16_t kInvalidTemperature = static_cast<std::int16_t>(-32768);

struct ThermalFrameMetadata {
    std::uint32_t sequence{0};
    std::uint32_t timestamp_ms{0};
    std::uint32_t calibration_revision{0};
};

struct ThermalFramePacket {
    ThermalFrameMetadata metadata{};
    std::array<float, kThermalPixelCount> temperatures_c{};
};

class ThermalFramePacketCodec {
public:
    static constexpr std::uint8_t kProtocolVersion = 1;

    [[nodiscard]] static std::vector<std::uint8_t> encode_binary(
        const ThermalFramePacket& packet);

    [[nodiscard]] static bool decode_binary(
        const std::vector<std::uint8_t>& bytes,
        ThermalFramePacket& packet_out);

    [[nodiscard]] static std::string encode_base64(
        const ThermalFramePacket& packet);

    [[nodiscard]] static bool decode_base64(
        const std::string& encoded,
        ThermalFramePacket& packet_out);

    [[nodiscard]] static std::uint32_t crc32(
        const std::uint8_t* data,
        std::size_t size);
};

}  // namespace leafsense::transport
