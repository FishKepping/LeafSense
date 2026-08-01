#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "amg8833_registers.h"
#include "amg8833_bus.h"
#include "thermal_frame.h"
#include "thermal_processor.h"

namespace leafsense::drivers {

/**
 * @brief Automatic recovery configuration.
 */
struct Amg8833RecoveryConfig
{
    bool enabled = true;

    /**
     * Values below 1 are treated as 1.
     */
    std::uint32_t failure_threshold = 3;
};

/**
 * @brief Hardware interrupt threshold configuration.
 *
 * Thresholds use the AMG8833 pixel-temperature resolution of 0.25 °C.
 *
 * Valid temperature range:
 *
 *     -512.0 °C through 511.75 °C
 *
 * Hysteresis must be non-negative and must not exceed the distance
 * between the lower and upper thresholds.
 */
struct Amg8833InterruptThresholds
{
    bool enabled = false;

    float upper_temperature = 30.0f;

    float lower_temperature = 10.0f;

    float hysteresis = 1.0f;
};

/**
 * @brief Complete hardware and processing configuration.
 */
struct Amg8833DriverConfig
{
    Amg8833FrameRate frame_rate =
        Amg8833FrameRate::FramesPerSecond10;

    bool moving_average_enabled = false;

    Amg8833InterruptConfig interrupt;

    Amg8833InterruptThresholds interrupt_thresholds;

    ProcessingConfig processing;

    Amg8833RecoveryConfig recovery;
};

/**
 * @brief Detailed AMG8833 driver operation errors.
 */
enum class Amg8833DriverError : std::uint8_t
{
    None,

    NotInitialized,

    InvalidInterruptThresholds,

    PowerControlWriteFailed,

    InitialResetWriteFailed,

    FrameRateWriteFailed,

    MovingAverageWriteFailed,

    InterruptThresholdWriteFailed,

    InterruptControlWriteFailed,

    StatusClearWriteFailed,

    StatusReadFailed,

    ThermistorReadFailed,

    PixelReadFailed,

    InterruptTableReadFailed
};

/**
 * @brief Result of one frame-acquisition attempt.
 */
struct Amg8833Acquisition
{
    ThermalFrame frame;

    Amg8833Status status;

    Amg8833DriverError error =
        Amg8833DriverError::None;

    bool recovery_attempted = false;

    bool recovery_succeeded = false;

    bool success() const;
};

/**
 * @brief Runtime health information.
 */
struct Amg8833DriverHealth
{
    bool initialized = false;

    std::uint32_t consecutive_failures = 0;

    std::uint32_t total_failures = 0;

    std::uint32_t recovery_attempts = 0;

    std::uint32_t successful_recoveries = 0;

    std::uint32_t failed_recoveries = 0;

    bool healthy() const;
};

/**
 * @brief Decoded 8×8 AMG8833 interrupt table.
 *
 * Pixel indexing follows ThermalFrame:
 *
 *     row 0, column 0 = pixel index 0
 *     row 7, column 7 = pixel index 63
 */
class Amg8833InterruptMap
{
public:
    static constexpr std::size_t WIDTH = 8;

    static constexpr std::size_t HEIGHT = 8;

    static constexpr std::size_t PIXEL_COUNT =
        WIDTH * HEIGHT;

    using RawBytes = std::array<std::uint8_t, 8>;

    Amg8833InterruptMap();

    /**
     * Return whether a pixel is active.
     *
     * Invalid coordinates return false.
     */
    bool active(
        std::size_t row,
        std::size_t column) const;

    /**
     * Return whether a linear pixel index is active.
     *
     * Invalid indices return false.
     */
    bool active(
        std::size_t pixel_index) const;

    /**
     * Return the number of active interrupt pixels.
     */
    std::size_t activeCount() const;

    /**
     * Return true when at least one pixel is active.
     */
    bool any() const;

    /**
     * Return true when no pixels are active.
     */
    bool empty() const;

    /**
     * Return the original interrupt-table bytes.
     */
    const RawBytes& rawBytes() const;

private:
    friend class Amg8833Driver;

    explicit Amg8833InterruptMap(
        const RawBytes& raw_bytes);

    RawBytes raw_bytes_;

    std::size_t active_count_;
};

/**
 * @brief Result of an interrupt-table read.
 */
struct Amg8833InterruptMapResult
{
    Amg8833InterruptMap map;

    Amg8833DriverError error =
        Amg8833DriverError::None;

    bool recovery_attempted = false;

    bool recovery_succeeded = false;

    bool success() const;
};

/**
 * @brief Platform-independent AMG8833 sensor driver.
 */
class Amg8833Driver
{
public:
    explicit Amg8833Driver(
        Amg8833Bus& bus);

    Amg8833Driver(
        Amg8833Bus& bus,
        const Amg8833DriverConfig& config);

    const Amg8833DriverConfig& config() const;

    bool initialize();

    bool initialized() const;

    void reset();

    Amg8833DriverError lastError() const;

    const Amg8833Status& lastStatus() const;

    std::uint32_t frameCount() const;

    Amg8833DriverHealth health() const;

    Amg8833Acquisition readFrame(
        std::uint32_t timestamp_ms);

    /**
     * Read and decode registers 0x10 through 0x17.
     *
     * A failed read participates in automatic recovery in the same way
     * as frame-acquisition communication failures.
     */
    Amg8833InterruptMapResult readInterruptMap();

    bool clearStatus();

    /**
     * Validate an interrupt-threshold configuration.
     */
    static bool validInterruptThresholds(
        const Amg8833InterruptThresholds& thresholds);

    /**
     * Encode a temperature as the sensor's signed 12-bit 0.25 °C
     * interrupt-threshold representation.
     *
     * Values outside the supported range are clamped.
     */
    static std::uint16_t encodeInterruptTemperature(
        float temperature);

private:
    bool performInitialization(
        bool preserve_health_counters);

    bool writeInitializationRegister(
        std::uint8_t register_address,
        std::uint8_t value,
        Amg8833DriverError error);

    bool configureMovingAverage();

    bool configureInterruptThresholds();

    bool writeInterruptTemperature(
        std::uint8_t low_register,
        std::uint8_t high_register,
        float temperature);

    void recordAcquisitionSuccess();

    void recordAcquisitionFailure(
        Amg8833Acquisition& acquisition);

    void recordInterruptMapSuccess();

    void recordInterruptMapFailure(
        Amg8833InterruptMapResult& result);

    std::uint32_t recoveryThreshold() const;

    Amg8833Bus& bus_;

    Amg8833DriverConfig config_;

    ThermalProcessor processor_;

    bool initialized_;

    Amg8833DriverError last_error_;

    Amg8833Status last_status_;

    std::uint32_t frame_count_;

    std::uint32_t consecutive_failures_;

    std::uint32_t total_failures_;

    std::uint32_t recovery_attempts_;

    std::uint32_t successful_recoveries_;

    std::uint32_t failed_recoveries_;
};

}  // namespace leafsense::drivers