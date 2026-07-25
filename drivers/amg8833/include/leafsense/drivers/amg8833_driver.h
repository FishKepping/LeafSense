#pragma once

#include <cstdint>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/thermal_frame.h"
#include "leafsense/thermal_processor.h"

namespace leafsense::drivers {

/**
 * @brief Automatic recovery configuration.
 */
struct Amg8833RecoveryConfig
{
    /**
     * Enable automatic sensor reinitialization after repeated
     * communication failures.
     */
    bool enabled = true;

    /**
     * Number of consecutive acquisition failures required before
     * automatic recovery is attempted.
     *
     * Values below 1 are treated as 1.
     */
    std::uint32_t failure_threshold = 3;
};

/**
 * @brief Complete hardware and processing configuration for an AMG8833.
 */
struct Amg8833DriverConfig
{
    Amg8833FrameRate frame_rate =
        Amg8833FrameRate::FramesPerSecond10;

    bool moving_average_enabled =
        false;

    Amg8833InterruptConfig interrupt;

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

    PowerControlWriteFailed,

    InitialResetWriteFailed,

    FrameRateWriteFailed,

    MovingAverageWriteFailed,

    InterruptControlWriteFailed,

    StatusClearWriteFailed,

    StatusReadFailed,

    ThermistorReadFailed,

    PixelReadFailed
};

/**
 * @brief Result of one AMG8833 frame-acquisition attempt.
 *
 * A failed read can trigger automatic recovery. Recovery information
 * is reported separately from the acquisition error because the frame
 * that triggered recovery was still not acquired.
 */
struct Amg8833Acquisition
{
    ThermalFrame frame;

    Amg8833Status status;

    Amg8833DriverError error =
        Amg8833DriverError::None;

    bool recovery_attempted =
        false;

    bool recovery_succeeded =
        false;

    /**
     * Return true when all bus reads completed successfully.
     */
    bool success() const;
};

/**
 * @brief Runtime health information for the AMG8833 driver.
 */
struct Amg8833DriverHealth
{
    bool initialized =
        false;

    std::uint32_t consecutive_failures =
        0;

    std::uint32_t total_failures =
        0;

    std::uint32_t recovery_attempts =
        0;

    std::uint32_t successful_recoveries =
        0;

    std::uint32_t failed_recoveries =
        0;

    /**
     * Return true when the driver is initialized and has no current
     * consecutive communication failures.
     */
    bool healthy() const;
};

/**
 * @brief Platform-independent AMG8833 sensor driver.
 *
 * The driver owns:
 *
 *     sensor initialization
 *     register acquisition
 *     frame processing
 *     status handling
 *     error tracking
 *     automatic recovery
 *
 * Automatic recovery is attempted after the configured number of
 * consecutive acquisition read failures.
 *
 * The acquisition that triggers recovery still returns its original
 * read error. The caller should request another frame after successful
 * recovery.
 *
 * Sensor overflow flags do not count as communication failures because
 * all register operations completed successfully.
 *
 * The implementation performs no heap allocation.
 */
class Amg8833Driver
{
public:
    /**
     * Construct a driver using default configuration.
     */
    explicit Amg8833Driver(
        Amg8833Bus& bus);

    /**
     * Construct a driver using the supplied configuration.
     */
    Amg8833Driver(
        Amg8833Bus& bus,
        const Amg8833DriverConfig& config);

    /**
     * Return the active configuration.
     */
    const Amg8833DriverConfig& config() const;

    /**
     * Configure and initialize the physical sensor.
     *
     * Frame numbering and temporal processing history are reset.
     * Lifetime health counters are preserved.
     */
    bool initialize();

    /**
     * Return true after successful initialization.
     */
    bool initialized() const;

    /**
     * Clear all local driver state and health counters.
     *
     * This does not write to the physical sensor.
     */
    void reset();

    /**
     * Return the most recent driver error.
     */
    Amg8833DriverError lastError() const;

    /**
     * Return the most recently decoded sensor status.
     */
    const Amg8833Status& lastStatus() const;

    /**
     * Return the number assigned to the most recently acquired frame.
     */
    std::uint32_t frameCount() const;

    /**
     * Return current driver health information.
     */
    Amg8833DriverHealth health() const;

    /**
     * Read, decode, and process one thermal frame.
     */
    Amg8833Acquisition readFrame(
        std::uint32_t timestamp_ms);

    /**
     * Clear the active flags from the most recently read status.
     *
     * No bus write is performed when no status flags are active.
     */
    bool clearStatus();

private:
    bool performInitialization(
        bool preserve_health_counters);

    bool writeInitializationRegister(
        std::uint8_t register_address,
        std::uint8_t value,
        Amg8833DriverError error);

    bool configureMovingAverage();

    void recordAcquisitionSuccess();

    void recordAcquisitionFailure(
        Amg8833Acquisition& acquisition);

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