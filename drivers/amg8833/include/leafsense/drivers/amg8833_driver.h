#pragma once

#include <cstdint>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/thermal_frame.h"
#include "leafsense/thermal_processor.h"

namespace leafsense::drivers {

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
 * success() indicates that all required bus reads completed.
 *
 * A successful acquisition can still contain an invalid ThermalFrame
 * when the sensor reports pixel or thermistor overflow. Sensor status
 * is therefore separate from the driver error.
 */
struct Amg8833Acquisition
{
    ThermalFrame frame;

    Amg8833Status status;

    Amg8833DriverError error =
        Amg8833DriverError::None;

    /**
     * Return true when all bus operations completed successfully.
     */
    bool success() const;
};

/**
 * @brief Platform-independent AMG8833 sensor driver.
 *
 * This class owns the sensor initialization and acquisition sequence,
 * while Amg8833Bus supplies platform-specific register access.
 *
 * Initialization performs:
 *
 *     normal power mode
 *     initial reset
 *     frame-rate configuration
 *     moving-average configuration
 *     interrupt configuration
 *     status clearing
 *
 * Frame acquisition performs:
 *
 *     status-register read
 *     thermistor-register read
 *     128-byte pixel-register read
 *     decoding and filtering through ThermalProcessor
 *
 * Pixel or thermistor overflow produces a successfully acquired but
 * invalid ThermalFrame. Invalid frames do not update exponential
 * filtering history.
 *
 * Frame numbers count complete register acquisitions, including frames
 * marked invalid because of sensor overflow. Failed bus reads do not
 * increment the frame number.
 *
 * initialize() performs register operations only. The platform adapter
 * remains responsible for ensuring the sensor has completed its
 * power-on communication setup period before initialization begins.
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
     * Driver state and temporal filtering history are cleared before
     * the initialization sequence begins.
     */
    bool initialize();

    /**
     * Return true after successful initialization.
     */
    bool initialized() const;

    /**
     * Clear local driver state.
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
    bool writeInitializationRegister(
        std::uint8_t register_address,
        std::uint8_t value,
        Amg8833DriverError error);

    bool configureMovingAverage();

    Amg8833Bus& bus_;

    Amg8833DriverConfig config_;

    ThermalProcessor processor_;

    bool initialized_;

    Amg8833DriverError last_error_;

    Amg8833Status last_status_;

    std::uint32_t frame_count_;
};

}  // namespace leafsense::drivers