#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "esphome_bus.h"
#include "measurement_channel_runtime_api.h"
#include "publisher.h"
#include "rectangle_roi_processor.h"

#include "thermal_calibrator.h"
#include "amg8833_driver.h"
#include "amg8833_snapshot.h"
#include "amg8833_telemetry.h"
#include "measurement_channel_controller.h"
#include "measurement_channel_manager.h"
#include "thermal_processing_pipeline.h"
#include "thermal_frame_publisher.h"

namespace esphome::leafsense_amg8833 {

class LeafSenseAmg8833Component
    : public PollingComponent,
      public i2c::I2CDevice
{
public:
    LeafSenseAmg8833Component();

    void setup() override;
    void update() override;
    void dump_config() override;
    float get_setup_priority() const override;

    void set_include_interrupt_map(bool value);
    void set_recovery_enabled(bool value);
    void set_recovery_failure_threshold(std::uint8_t value);
    void set_moving_average_enabled(bool value);
    void set_dead_pixel_correction_enabled(bool value);
    void set_temporal_smoothing_enabled(bool value);
    void set_temporal_smoothing_alpha(float value);
    void set_spatial_median_enabled(bool value);

    void set_rectangle_roi(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t width,
        std::uint8_t height);

    bool set_calibration_gain(float value);
    bool set_calibration_offset(float value);
    bool save_calibration();
    void restore_calibration_defaults();
    float calibration_gain() const;
    float calibration_offset() const;
    std::uint32_t calibration_revision() const;
    float current_frame_average() const;

    float channel_minimum(std::size_t index) const;
    float channel_maximum(std::size_t index) const;
    float channel_average(std::size_t index) const;
    float channel_pixel_count(std::size_t index) const;

    void set_thermal_frame_text_sensor(text_sensor::TextSensor* value);

    void set_minimum_temperature_sensor(sensor::Sensor* value);
    void set_maximum_temperature_sensor(sensor::Sensor* value);
    void set_average_temperature_sensor(sensor::Sensor* value);
    void set_thermistor_temperature_sensor(sensor::Sensor* value);
    void set_roi_minimum_temperature_sensor(sensor::Sensor* value);
    void set_roi_maximum_temperature_sensor(sensor::Sensor* value);
    void set_roi_average_temperature_sensor(sensor::Sensor* value);
    void set_roi_pixel_count_sensor(sensor::Sensor* value);
    void set_frame_count_sensor(sensor::Sensor* value);
    void set_valid_pixel_count_sensor(sensor::Sensor* value);
    void set_active_interrupt_pixel_count_sensor(sensor::Sensor* value);
    void set_consecutive_failures_sensor(sensor::Sensor* value);
    void set_total_failures_sensor(sensor::Sensor* value);
    void set_recovery_attempts_sensor(sensor::Sensor* value);
    void set_successful_recoveries_sensor(sensor::Sensor* value);
    void set_failed_recoveries_sensor(sensor::Sensor* value);

    void set_connected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_driver_problem_binary_sensor(binary_sensor::BinarySensor* value);
    void set_frame_available_binary_sensor(binary_sensor::BinarySensor* value);
    void set_overflow_detected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_interrupt_detected_binary_sensor(binary_sensor::BinarySensor* value);
    void set_recovery_active_binary_sensor(binary_sensor::BinarySensor* value);
    void set_roi_available_binary_sensor(binary_sensor::BinarySensor* value);

private:
    bool initializeDriver_();
    void loadCalibration_();
    void updateProcessedSummary_(leafsense::drivers::Amg8833Snapshot& snapshot);
    static const char* driverErrorName_(leafsense::drivers::Amg8833DriverError error);
    static float unavailableValue_();

    static constexpr std::uint32_t CALIBRATION_PREFERENCE_KEY = 0x4C533300U;

    bool include_interrupt_map_ = false;
    leafsense::drivers::Amg8833DriverConfig driver_config_{};

    EspHomeAmg8833Bus bus_adapter_;
    std::unique_ptr<leafsense::drivers::Amg8833Driver> driver_;
    std::unique_ptr<leafsense::drivers::Amg8833SnapshotReader> snapshot_reader_;

    leafsense::calibration::ThermalCalibrator calibrator_{};
    ESPPreferenceObject calibration_preference_{};
    leafsense::processing::ThermalProcessingPipeline processing_pipeline_{};
    leafsense::processing::ThermalProcessingOptions processing_options_{};

    leafsense::measurement::MeasurementChannelManager channel_manager_{};
    leafsense::measurement::MeasurementChannelController channel_controller_;
    MeasurementChannelRuntimeApi runtime_api_{};
    std::array<leafsense::measurement::MeasurementChannelResult,
               leafsense::measurement::MeasurementChannelManager::CHANNEL_COUNT>
        channel_results_{};

    text_sensor::TextSensor* thermal_frame_text_sensor_ = nullptr;
    leafsense::transport::ThermalFramePublisher frame_publisher_;
    float current_frame_average_ = 0.0F;

    RectangleRoiProcessor roi_processor_{};
    Amg8833TelemetryPublisher publisher_{};
};

}  // namespace esphome::leafsense_amg8833
