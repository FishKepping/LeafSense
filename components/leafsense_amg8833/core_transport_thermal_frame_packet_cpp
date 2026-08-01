#include "leafsense/transport/thermal_frame_packet.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace leafsense::transport {
namespace {

constexpr std::uint8_t kMagic0 = 'L';
constexpr std::uint8_t kMagic1 = 'S';
constexpr std::size_t kHeaderSize = 24;
constexpr std::size_t kPixelBytes = kThermalPixelCount * sizeof(std::int16_t);
constexpr std::size_t kCrcSize = sizeof(std::uint32_t);
constexpr std::size_t kPacketSize = kHeaderSize + kPixelBytes + kCrcSize;

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void append_i16(std::vector<std::uint8_t>& out, std::int16_t value) {
    append_u16(out, static_cast<std::uint16_t>(value));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& in, std::size_t offset) {
    return static_cast<std::uint16_t>(in[offset]) |
           (static_cast<std::uint16_t>(in[offset + 1]) << 8U);
}

std::int16_t read_i16(const std::vector<std::uint8_t>& in, std::size_t offset) {
    return static_cast<std::int16_t>(read_u16(in, offset));
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& in, std::size_t offset) {
    return static_cast<std::uint32_t>(in[offset]) |
           (static_cast<std::uint32_t>(in[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(in[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(in[offset + 3]) << 24U);
}

std::int16_t quantize(float value) {
    if (!std::isfinite(value)) {
        return kInvalidTemperature;
    }
    const float scaled = std::round(value * 100.0F);
    const float low = static_cast<float>(std::numeric_limits<std::int16_t>::min() + 1);
    const float high = static_cast<float>(std::numeric_limits<std::int16_t>::max());
    return static_cast<std::int16_t>(std::clamp(scaled, low, high));
}

float dequantize(std::int16_t value) {
    if (value == kInvalidTemperature) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return static_cast<float>(value) / 100.0F;
}

const char* kBase64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::vector<std::uint8_t>& input) {
    std::string output;
    output.reserve(((input.size() + 2U) / 3U) * 4U);

    for (std::size_t i = 0; i < input.size(); i += 3U) {
        const std::uint32_t a = input[i];
        const std::uint32_t b = (i + 1U < input.size()) ? input[i + 1U] : 0U;
        const std::uint32_t c = (i + 2U < input.size()) ? input[i + 2U] : 0U;
        const std::uint32_t triple = (a << 16U) | (b << 8U) | c;

        output.push_back(kBase64Alphabet[(triple >> 18U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(triple >> 12U) & 0x3FU]);
        output.push_back(i + 1U < input.size()
                             ? kBase64Alphabet[(triple >> 6U) & 0x3FU]
                             : '=');
        output.push_back(i + 2U < input.size()
                             ? kBase64Alphabet[triple & 0x3FU]
                             : '=');
    }
    return output;
}

int decode_base64_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool base64_decode(const std::string& input, std::vector<std::uint8_t>& output) {
    if (input.empty() || input.size() % 4U != 0U) return false;
    output.clear();
    output.reserve((input.size() / 4U) * 3U);

    for (std::size_t i = 0; i < input.size(); i += 4U) {
        const int a = decode_base64_char(input[i]);
        const int b = decode_base64_char(input[i + 1U]);
        const int c = input[i + 2U] == '=' ? 0 : decode_base64_char(input[i + 2U]);
        const int d = input[i + 3U] == '=' ? 0 : decode_base64_char(input[i + 3U]);
        if (a < 0 || b < 0 || c < 0 || d < 0) return false;

        const std::uint32_t triple =
            (static_cast<std::uint32_t>(a) << 18U) |
            (static_cast<std::uint32_t>(b) << 12U) |
            (static_cast<std::uint32_t>(c) << 6U) |
            static_cast<std::uint32_t>(d);

        output.push_back(static_cast<std::uint8_t>((triple >> 16U) & 0xFFU));
        if (input[i + 2U] != '=') {
            output.push_back(static_cast<std::uint8_t>((triple >> 8U) & 0xFFU));
        }
        if (input[i + 3U] != '=') {
            output.push_back(static_cast<std::uint8_t>(triple & 0xFFU));
        }
    }
    return true;
}

}  // namespace

std::vector<std::uint8_t> ThermalFramePacketCodec::encode_binary(
    const ThermalFramePacket& packet) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(kPacketSize);

    bytes.push_back(kMagic0);
    bytes.push_back(kMagic1);
    bytes.push_back(kProtocolVersion);
    bytes.push_back(0U);
    append_u32(bytes, packet.metadata.sequence);
    append_u32(bytes, packet.metadata.timestamp_ms);
    append_u32(bytes, packet.metadata.calibration_revision);
    bytes.push_back(static_cast<std::uint8_t>(kThermalFrameWidth));
    bytes.push_back(static_cast<std::uint8_t>(kThermalFrameHeight));
    append_u16(bytes, 0U);

    std::int16_t minimum = std::numeric_limits<std::int16_t>::max();
    std::int16_t maximum = std::numeric_limits<std::int16_t>::min();
    bool has_valid = false;
    std::array<std::int16_t, kThermalPixelCount> samples{};

    for (std::size_t i = 0; i < packet.temperatures_c.size(); ++i) {
        samples[i] = quantize(packet.temperatures_c[i]);
        if (samples[i] != kInvalidTemperature) {
            minimum = std::min(minimum, samples[i]);
            maximum = std::max(maximum, samples[i]);
            has_valid = true;
        }
    }

    append_i16(bytes, has_valid ? minimum : kInvalidTemperature);
    append_i16(bytes, has_valid ? maximum : kInvalidTemperature);

    for (const auto sample : samples) append_i16(bytes, sample);

    const auto checksum = crc32(bytes.data(), bytes.size());
    append_u32(bytes, checksum);
    return bytes;
}

bool ThermalFramePacketCodec::decode_binary(
    const std::vector<std::uint8_t>& bytes,
    ThermalFramePacket& packet_out) {
    if (bytes.size() != kPacketSize) return false;
    if (bytes[0] != kMagic0 || bytes[1] != kMagic1) return false;
    if (bytes[2] != kProtocolVersion) return false;
    if (bytes[16] != kThermalFrameWidth || bytes[17] != kThermalFrameHeight) return false;

    const auto expected_crc = read_u32(bytes, bytes.size() - kCrcSize);
    const auto actual_crc = crc32(bytes.data(), bytes.size() - kCrcSize);
    if (expected_crc != actual_crc) return false;

    packet_out.metadata.sequence = read_u32(bytes, 4U);
    packet_out.metadata.timestamp_ms = read_u32(bytes, 8U);
    packet_out.metadata.calibration_revision = read_u32(bytes, 12U);

    std::size_t offset = kHeaderSize;
    for (auto& temperature : packet_out.temperatures_c) {
        temperature = dequantize(read_i16(bytes, offset));
        offset += sizeof(std::int16_t);
    }
    return true;
}

std::string ThermalFramePacketCodec::encode_base64(
    const ThermalFramePacket& packet) {
    return base64_encode(encode_binary(packet));
}

bool ThermalFramePacketCodec::decode_base64(
    const std::string& encoded,
    ThermalFramePacket& packet_out) {
    std::vector<std::uint8_t> bytes;
    return base64_decode(encoded, bytes) && decode_binary(bytes, packet_out);
}

std::uint32_t ThermalFramePacketCodec::crc32(
    const std::uint8_t* data,
    std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

}  // namespace leafsense::transport
