#include "leafsense/thermal_frame.h"

namespace leafsense {

ThermalFrame::ThermalFrame() {
    clear();
}

void ThermalFrame::clear() {
    pixels_.fill(0.0f);

    thermistor_temperature_ = 0.0f;

    frame_number_ = 0;

    timestamp_ms_ = 0;

    valid_ = false;
}

bool ThermalFrame::isValid() const {
    return valid_;
}

void ThermalFrame::setValid(bool valid) {
    valid_ = valid;
}

uint32_t ThermalFrame::frameNumber() const {
    return frame_number_;
}

void ThermalFrame::setFrameNumber(uint32_t frame) {
    frame_number_ = frame;
}

uint32_t ThermalFrame::timestampMs() const {
    return timestamp_ms_;
}

void ThermalFrame::setTimestampMs(uint32_t timestamp) {
    timestamp_ms_ = timestamp;
}

float ThermalFrame::thermistorTemperature() const {
    return thermistor_temperature_;
}

void ThermalFrame::setThermistorTemperature(float temperature) {
    thermistor_temperature_ = temperature;
}

float ThermalFrame::pixel(uint8_t x, uint8_t y) const {
    if (x >= WIDTH || y >= HEIGHT) {
        return 0.0f;
    }

    return pixels_[y * WIDTH + x];
}

void ThermalFrame::setPixel(uint8_t x, uint8_t y, float temperature) {
    if (x >= WIDTH || y >= HEIGHT) {
        return;
    }

    pixels_[y * WIDTH + x] = temperature;
}

} // namespace leafsense
