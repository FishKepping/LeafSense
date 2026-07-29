#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "leafsense/transport/thermal_frame_packet.h"
#include "leafsense/transport/thermal_frame_publisher.h"

#include <cmath>
#include <limits>
#include <string>

using Catch::Approx;
using leafsense::transport::ThermalFramePacket;
using leafsense::transport::ThermalFramePacketCodec;
using leafsense::transport::ThermalFramePublisher;
using leafsense::transport::kThermalPixelCount;

TEST_CASE("thermal frame packet survives binary round trip") {
    ThermalFramePacket source{};
    source.metadata.sequence = 42;
    source.metadata.timestamp_ms = 123456;
    source.metadata.calibration_revision = 7;
    for (std::size_t i = 0; i < kThermalPixelCount; ++i) {
        source.temperatures_c[i] = 18.0F + static_cast<float>(i) * 0.25F;
    }

    const auto encoded = ThermalFramePacketCodec::encode_binary(source);
    ThermalFramePacket decoded{};
    REQUIRE(ThermalFramePacketCodec::decode_binary(encoded, decoded));
    CHECK(decoded.metadata.sequence == 42);
    CHECK(decoded.metadata.timestamp_ms == 123456);
    CHECK(decoded.metadata.calibration_revision == 7);
    for (std::size_t i = 0; i < kThermalPixelCount; ++i) {
        CHECK(decoded.temperatures_c[i] == Approx(source.temperatures_c[i]).margin(0.01));
    }
}

TEST_CASE("base64 payload remains compact enough for transport") {
    ThermalFramePacket packet{};
    packet.temperatures_c.fill(25.0F);
    const auto payload = ThermalFramePacketCodec::encode_base64(packet);
    CHECK(payload.size() <= 220);

    ThermalFramePacket decoded{};
    REQUIRE(ThermalFramePacketCodec::decode_base64(payload, decoded));
    CHECK(decoded.temperatures_c[0] == Approx(25.0F));
}

TEST_CASE("corrupted packets are rejected by CRC") {
    ThermalFramePacket packet{};
    packet.temperatures_c.fill(21.5F);
    auto encoded = ThermalFramePacketCodec::encode_binary(packet);
    encoded[30] ^= 0x01U;

    ThermalFramePacket decoded{};
    CHECK_FALSE(ThermalFramePacketCodec::decode_binary(encoded, decoded));
}

TEST_CASE("invalid temperatures round trip as NaN") {
    ThermalFramePacket packet{};
    packet.temperatures_c.fill(20.0F);
    packet.temperatures_c[10] = std::numeric_limits<float>::quiet_NaN();

    ThermalFramePacket decoded{};
    REQUIRE(ThermalFramePacketCodec::decode_base64(
        ThermalFramePacketCodec::encode_base64(packet), decoded));
    CHECK(std::isnan(decoded.temperatures_c[10]));
}

TEST_CASE("publisher increments sequence and forwards payload") {
    std::string last_payload;
    ThermalFramePublisher publisher(
        [&last_payload](const std::string& payload) { last_payload = payload; });
    publisher.set_calibration_revision(3);

    std::array<float, kThermalPixelCount> frame{};
    frame.fill(24.25F);
    REQUIRE(publisher.publish(frame, 1000));
    CHECK(publisher.sequence() == 1);
    REQUIRE_FALSE(last_payload.empty());

    ThermalFramePacket decoded{};
    REQUIRE(ThermalFramePacketCodec::decode_base64(last_payload, decoded));
    CHECK(decoded.metadata.sequence == 1);
    CHECK(decoded.metadata.timestamp_ms == 1000);
    CHECK(decoded.metadata.calibration_revision == 3);
}
