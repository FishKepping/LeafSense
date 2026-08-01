#include "leafsense_component.h"

#include <cmath>
#include <limits>
#include <utility>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::leafsense_amg8833 {
namespace {
static const char* const TAG = "leafsense_amg8833";
}

LeafSenseAmg8833Component::LeafSenseAmg8833Component()
    : bus_adapter_(*this),
      channel_controller_(channel_manager_),
      frame_publisher_([this](const std::string& payload) {
          if (thermal_frame_text_sensor_ != nullptr) {
              thermal_frame_text_sensor_->publish_state(payload);
          }
      })
{
    runtime_api_.set_controller(&channel_controller_);
}

void LeafSenseAmg8833Component::setup()
{
    ESP_LOGCONFIG(TAG, "Setting up LeafSense AMG8833...");
    loadCalibration_();
    runtime_api_.setup();

    if (!initializeDriver_()) {
        status_set_error();
        publisher_.publishUnavailable();
        ESP_LOGE(TAG, "AMG8833 initialization failed: %s",
                 driverErrorName_(driver_ != nullptr
                     ? driver_->lastError()
                     : leafsense::drivers::Amg8833DriverError::NotInitialized));
        return;
    }

    status_clear_error();
    ESP_LOGCONFIG(TAG, "LeafSense AMG8833 driver initialized");
}

void LeafSenseAmg8833Component::update()
{
    if (driver_ == nullptr || snapshot_reader_ == nullptr || !driver_->initialized()) {
        if (!initializeDriver_()) {
            status_set_error();
            publisher_.publishUnavailable();
            ESP_LOGW(TAG, "AMG8833 reinitialization failed: %s",
                     driverErrorName_(driver_ != nullptr
                         ? driver_->lastError()
                         : leafsense::drivers::Amg8833DriverError::NotInitialized));
            return;
        }
    }

    leafsense::drivers::Amg8833Snapshot snapshot =
        snapshot_reader_->capture(millis(), include_interrupt_map_);

    if (snapshot.frameAvailable()) {
        calibrator_.applyInPlace(snapshot.frame);
        snapshot.frame = processing_pipeline_.process(snapshot.frame, processing_options_);
        updateProcessedSummary_(snapshot);

        channel_results_ = channel_manager_.process(snapshot.frame);
        frame_publisher_.set_calibration_revision(calibrator_.settings().revision);
        frame_publisher_.publish(snapshot.frame.pixels(), snapshot.frame.timestampMs());
    } else {
        channel_results_ = {};
    }

    const auto telemetry = leafsense::drivers::Amg8833TelemetryProjector::project(snapshot);
    const RectangleRoiResult roi_result = roi_processor_.process(snapshot.frame);
    publisher_.publish(telemetry, roi_result);

    if (telemetry.frame_read_succeeded) {
        status_clear_warning();
        status_clear_error();
    } else {
        status_set_warning();
        ESP_LOGW(TAG, "AMG8833 capture failed: %s", driverErrorName_(telemetry.frame_error));
    }

    if (telemetry.recovery_attempted) {
        ESP_LOGI(TAG, "Automatic recovery attempted: %s",
                 telemetry.recovery_succeeded ? "succeeded" : "failed");
    }
}

void LeafSenseAmg8833Component::dump_config()
{
    ESP_LOGCONFIG(TAG, "LeafSense AMG8833:");
    LOG_I2C_DEVICE(this);
    LOG_UPDATE_INTERVAL(this);
    ESP_LOGCONFIG(TAG, "  Interrupt map: %s", include_interrupt_map_ ? "enabled" : "disabled");
    ESP_LOGCONFIG(TAG, "  Driver moving average: %s", driver_config_.moving_average_enabled ? "enabled" : "disabled");
    ESP_LOGCONFIG(TAG, "  Dead-pixel correction: %s", processing_options_.dead_pixel.enabled ? "enabled" : "disabled");
    ESP_LOGCONFIG(TAG, "  Temporal smoothing: %s (alpha %.3f)",
                  processing_options_.temporal.enabled ? "enabled" : "disabled",
                  processing_options_.temporal.alpha);
    ESP_LOGCONFIG(TAG, "  Spatial median: %s", processing_options_.spatial.enabled ? "enabled" : "disabled");
    ESP_LOGCONFIG(TAG, "  Calibration gain %.4f, offset %.3f C, revision %u",
                  calibrator_.settings().gain,
                  calibrator_.settings().offset_c,
                  static_cast<unsigned>(calibrator_.settings().revision));
    runtime_api_.dump_config();
}

float LeafSenseAmg8833Component::get_setup_priority() const { return setup_priority::DATA; }

bool LeafSenseAmg8833Component::initializeDriver_()
{
    driver_ = std::make_unique<leafsense::drivers::Amg8833Driver>(bus_adapter_, driver_config_);
    if (!driver_->initialize()) {
        snapshot_reader_.reset();
        return false;
    }
    snapshot_reader_ = std::make_unique<leafsense::drivers::Amg8833SnapshotReader>(*driver_);
    return true;
}

void LeafSenseAmg8833Component::loadCalibration_()
{
    calibration_preference_ = global_preferences->make_preference<leafsense::calibration::CalibrationSettings>(CALIBRATION_PREFERENCE_KEY);
    leafsense::calibration::CalibrationSettings stored{};
    if (calibration_preference_.load(&stored) && calibrator_.setSettings(stored)) {
        ESP_LOGI(TAG, "Loaded calibration revision %u", static_cast<unsigned>(stored.revision));
    } else {
        ESP_LOGI(TAG, "No valid saved calibration; using defaults");
    }
}

void LeafSenseAmg8833Component::updateProcessedSummary_(
    leafsense::drivers::Amg8833Snapshot& snapshot)
{
    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();
    double total = 0.0;
    std::size_t count = 0U;

    for (std::size_t y = 0; y < leafsense::ThermalFrame::HEIGHT; ++y) {
        for (std::size_t x = 0; x < leafsense::ThermalFrame::WIDTH; ++x) {
            const auto px = static_cast<std::uint8_t>(x);
            const auto py = static_cast<std::uint8_t>(y);
            const float value = snapshot.frame.pixel(px, py);
            if (!snapshot.frame.pixelValid(px, py) || !std::isfinite(value)) continue;
            minimum = value < minimum ? value : minimum;
            maximum = value > maximum ? value : maximum;
            total += value;
            ++count;
        }
    }

    snapshot.summary.available = count > 0U;
    snapshot.summary.valid_pixel_count = count;
    snapshot.summary.thermistor_temperature = snapshot.frame.thermistorTemperature();
    if (count > 0U) {
        snapshot.summary.minimum_temperature = minimum;
        snapshot.summary.maximum_temperature = maximum;
        snapshot.summary.average_temperature = static_cast<float>(total / static_cast<double>(count));
        current_frame_average_ = snapshot.summary.average_temperature;
    } else {
        current_frame_average_ = unavailableValue_();
    }
}

void LeafSenseAmg8833Component::set_include_interrupt_map(bool v) { include_interrupt_map_ = v; }
void LeafSenseAmg8833Component::set_recovery_enabled(bool v) { driver_config_.recovery.enabled = v; }
void LeafSenseAmg8833Component::set_recovery_failure_threshold(std::uint8_t v) { driver_config_.recovery.failure_threshold = v == 0U ? 1U : v; }
void LeafSenseAmg8833Component::set_moving_average_enabled(bool v) { driver_config_.moving_average_enabled = v; }
void LeafSenseAmg8833Component::set_dead_pixel_correction_enabled(bool v) { processing_options_.dead_pixel.enabled = v; }
void LeafSenseAmg8833Component::set_temporal_smoothing_enabled(bool v) { processing_options_.temporal.enabled = v; }
void LeafSenseAmg8833Component::set_temporal_smoothing_alpha(float v) { if (std::isfinite(v) && v >= 0.0F && v <= 1.0F) processing_options_.temporal.alpha = v; }
void LeafSenseAmg8833Component::set_spatial_median_enabled(bool v) { processing_options_.spatial.enabled = v; }
void LeafSenseAmg8833Component::set_rectangle_roi(std::uint8_t x, std::uint8_t y, std::uint8_t w, std::uint8_t h) { roi_processor_.configure(x, y, w, h); }

bool LeafSenseAmg8833Component::set_calibration_gain(float v) { return calibrator_.setGain(v); }
bool LeafSenseAmg8833Component::set_calibration_offset(float v) { return calibrator_.setOffset(v); }
bool LeafSenseAmg8833Component::save_calibration() { const auto settings = calibrator_.settings(); return calibration_preference_.save(&settings); }
void LeafSenseAmg8833Component::restore_calibration_defaults() { calibrator_.restoreDefaults(); }
float LeafSenseAmg8833Component::calibration_gain() const { return calibrator_.settings().gain; }
float LeafSenseAmg8833Component::calibration_offset() const { return calibrator_.settings().offset_c; }
std::uint32_t LeafSenseAmg8833Component::calibration_revision() const { return calibrator_.settings().revision; }
float LeafSenseAmg8833Component::current_frame_average() const { return current_frame_average_; }

float LeafSenseAmg8833Component::channel_minimum(std::size_t i) const { return i < channel_results_.size() && channel_results_[i].available ? channel_results_[i].minimum_temperature : unavailableValue_(); }
float LeafSenseAmg8833Component::channel_maximum(std::size_t i) const { return i < channel_results_.size() && channel_results_[i].available ? channel_results_[i].maximum_temperature : unavailableValue_(); }
float LeafSenseAmg8833Component::channel_average(std::size_t i) const { return i < channel_results_.size() && channel_results_[i].available ? channel_results_[i].average_temperature : unavailableValue_(); }
float LeafSenseAmg8833Component::channel_pixel_count(std::size_t i) const { return i < channel_results_.size() && channel_results_[i].available ? static_cast<float>(channel_results_[i].valid_pixel_count) : unavailableValue_(); }
float LeafSenseAmg8833Component::unavailableValue_() { return std::numeric_limits<float>::quiet_NaN(); }

void LeafSenseAmg8833Component::set_thermal_frame_text_sensor(text_sensor::TextSensor* v) { thermal_frame_text_sensor_ = v; }

#define LS_SENSOR_SETTER(method, publisher_method) \
void LeafSenseAmg8833Component::method(sensor::Sensor* v) { publisher_.publisher_method(v); }
LS_SENSOR_SETTER(set_minimum_temperature_sensor, setMinimumTemperatureSensor)
LS_SENSOR_SETTER(set_maximum_temperature_sensor, setMaximumTemperatureSensor)
LS_SENSOR_SETTER(set_average_temperature_sensor, setAverageTemperatureSensor)
LS_SENSOR_SETTER(set_thermistor_temperature_sensor, setThermistorTemperatureSensor)
LS_SENSOR_SETTER(set_roi_minimum_temperature_sensor, setRoiMinimumTemperatureSensor)
LS_SENSOR_SETTER(set_roi_maximum_temperature_sensor, setRoiMaximumTemperatureSensor)
LS_SENSOR_SETTER(set_roi_average_temperature_sensor, setRoiAverageTemperatureSensor)
LS_SENSOR_SETTER(set_roi_pixel_count_sensor, setRoiPixelCountSensor)
LS_SENSOR_SETTER(set_frame_count_sensor, setFrameCountSensor)
LS_SENSOR_SETTER(set_valid_pixel_count_sensor, setValidPixelCountSensor)
LS_SENSOR_SETTER(set_active_interrupt_pixel_count_sensor, setActiveInterruptPixelCountSensor)
LS_SENSOR_SETTER(set_consecutive_failures_sensor, setConsecutiveFailuresSensor)
LS_SENSOR_SETTER(set_total_failures_sensor, setTotalFailuresSensor)
LS_SENSOR_SETTER(set_recovery_attempts_sensor, setRecoveryAttemptsSensor)
LS_SENSOR_SETTER(set_successful_recoveries_sensor, setSuccessfulRecoveriesSensor)
LS_SENSOR_SETTER(set_failed_recoveries_sensor, setFailedRecoveriesSensor)
#undef LS_SENSOR_SETTER

#define LS_BINARY_SETTER(method, publisher_method) \
void LeafSenseAmg8833Component::method(binary_sensor::BinarySensor* v) { publisher_.publisher_method(v); }
LS_BINARY_SETTER(set_connected_binary_sensor, setConnectedBinarySensor)
LS_BINARY_SETTER(set_driver_problem_binary_sensor, setDriverProblemBinarySensor)
LS_BINARY_SETTER(set_frame_available_binary_sensor, setFrameAvailableBinarySensor)
LS_BINARY_SETTER(set_overflow_detected_binary_sensor, setOverflowDetectedBinarySensor)
LS_BINARY_SETTER(set_interrupt_detected_binary_sensor, setInterruptDetectedBinarySensor)
LS_BINARY_SETTER(set_recovery_active_binary_sensor, setRecoveryActiveBinarySensor)
LS_BINARY_SETTER(set_roi_available_binary_sensor, setRoiAvailableBinarySensor)
#undef LS_BINARY_SETTER

const char* LeafSenseAmg8833Component::driverErrorName_(
    leafsense::drivers::Amg8833DriverError error)
{
    using E = leafsense::drivers::Amg8833DriverError;

    switch (error)
    {
        case E::None:
            return "none";

        case E::NotInitialized:
            return "not_initialized";

        case E::InvalidInterruptThresholds:
            return "invalid_interrupt_thresholds";

        case E::PowerControlWriteFailed:
            return "power_control_write_failed";

        case E::InitialResetWriteFailed:
            return "initial_reset_write_failed";

        case E::FrameRateWriteFailed:
            return "frame_rate_write_failed";

        case E::MovingAverageWriteFailed:
            return "moving_average_write_failed";

        case E::InterruptThresholdWriteFailed:
            return "interrupt_threshold_write_failed";

        case E::InterruptControlWriteFailed:
            return "interrupt_control_write_failed";

        case E::StatusClearWriteFailed:
            return "status_clear_write_failed";

        case E::StatusReadFailed:
            return "status_read_failed";

        case E::PixelReadFailed:
            return "pixel_read_failed";

        case E::InterruptTableReadFailed:
            return "interrupt_table_read_failed";

        case E::ThermistorReadFailed:
            return "thermistor_read_failed";
    }

    return "unknown";
}

}  // namespace esphome::leafsense_amg8833