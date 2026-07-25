#include "leafsense/drivers/amg8833_driver.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "leafsense/amg8833_decoder.h"
#include "leafsense/amg8833_registers.h"

namespace leafsense::drivers {

namespace {

constexpr std::uint8_t MOVING_AVERAGE_UNLOCK_VALUE_1 =
    0x50;

constexpr std::uint8_t MOVING_AVERAGE_UNLOCK_VALUE_2 =
    0x45;

constexpr std::uint8_t MOVING_AVERAGE_UNLOCK_VALUE_3 =
    0x57;

constexpr std::uint8_t MOVING_AVERAGE_LOCK_VALUE =
    0x00;

constexpr float INTERRUPT_RESOLUTION =
    0.25f;

constexpr float MINIMUM_INTERRUPT_TEMPERATURE =
    -512.0f;

constexpr float MAXIMUM_INTERRUPT_TEMPERATURE =
    511.75f;

constexpr std::int32_t MINIMUM_INTERRUPT_COUNTS =
    -2048;

constexpr std::int32_t MAXIMUM_INTERRUPT_COUNTS =
    2047;

}  // namespace

bool Amg8833Acquisition::success() const
{
    return error ==
           Amg8833DriverError::None;
}

bool Amg8833DriverHealth::healthy() const
{
    return initialized &&
           consecutive_failures == 0U;
}

Amg8833InterruptMap::Amg8833InterruptMap()
    : raw_bytes_(),
      active_count_(0)
{
}

Amg8833InterruptMap::Amg8833InterruptMap(
    const RawBytes& raw_bytes)
    : raw_bytes_(raw_bytes),
      active_count_(0)
{
    for (std::uint8_t byte :
         raw_bytes_)
    {
        for (std::size_t bit = 0;
             bit < 8;
             ++bit)
        {
            if ((byte &
                 static_cast<std::uint8_t>(
                     1U << bit)) != 0U)
            {
                ++active_count_;
            }
        }
    }
}

bool Amg8833InterruptMap::active(
    std::size_t row,
    std::size_t column) const
{
    if (row >= HEIGHT ||
        column >= WIDTH)
    {
        return false;
    }

    return active(
        row * WIDTH + column);
}

bool Amg8833InterruptMap::active(
    std::size_t pixel_index) const
{
    if (pixel_index >= PIXEL_COUNT)
    {
        return false;
    }

    const std::size_t byte_index =
        pixel_index / 8;

    const std::size_t bit_index =
        pixel_index % 8;

    const std::uint8_t mask =
        static_cast<std::uint8_t>(
            1U << bit_index);

    return (raw_bytes_[byte_index] &
            mask) != 0U;
}

std::size_t
Amg8833InterruptMap::activeCount() const
{
    return active_count_;
}

bool Amg8833InterruptMap::any() const
{
    return active_count_ > 0U;
}

bool Amg8833InterruptMap::empty() const
{
    return !any();
}

const Amg8833InterruptMap::RawBytes&
Amg8833InterruptMap::rawBytes() const
{
    return raw_bytes_;
}

bool Amg8833InterruptMapResult::success() const
{
    return error ==
           Amg8833DriverError::None;
}

Amg8833Driver::Amg8833Driver(
    Amg8833Bus& bus)
    : Amg8833Driver(
          bus,
          Amg8833DriverConfig{})
{
}

Amg8833Driver::Amg8833Driver(
    Amg8833Bus& bus,
    const Amg8833DriverConfig& config)
    : bus_(bus),
      config_(config),
      processor_(config.processing),
      initialized_(false),
      last_error_(Amg8833DriverError::None),
      last_status_(),
      frame_count_(0),
      consecutive_failures_(0),
      total_failures_(0),
      recovery_attempts_(0),
      successful_recoveries_(0),
      failed_recoveries_(0)
{
}

const Amg8833DriverConfig&
Amg8833Driver::config() const
{
    return config_;
}

bool Amg8833Driver::initialize()
{
    return performInitialization(
        true);
}

bool Amg8833Driver::initialized() const
{
    return initialized_;
}

void Amg8833Driver::reset()
{
    initialized_ = false;

    last_error_ =
        Amg8833DriverError::None;

    last_status_ =
        Amg8833Status{};

    frame_count_ = 0;

    consecutive_failures_ = 0;

    total_failures_ = 0;

    recovery_attempts_ = 0;

    successful_recoveries_ = 0;

    failed_recoveries_ = 0;

    processor_.reset();
}

Amg8833DriverError
Amg8833Driver::lastError() const
{
    return last_error_;
}

const Amg8833Status&
Amg8833Driver::lastStatus() const
{
    return last_status_;
}

std::uint32_t
Amg8833Driver::frameCount() const
{
    return frame_count_;
}

Amg8833DriverHealth
Amg8833Driver::health() const
{
    Amg8833DriverHealth result;

    result.initialized =
        initialized_;

    result.consecutive_failures =
        consecutive_failures_;

    result.total_failures =
        total_failures_;

    result.recovery_attempts =
        recovery_attempts_;

    result.successful_recoveries =
        successful_recoveries_;

    result.failed_recoveries =
        failed_recoveries_;

    return result;
}

Amg8833Acquisition Amg8833Driver::readFrame(
    std::uint32_t timestamp_ms)
{
    Amg8833Acquisition acquisition;

    if (!initialized_)
    {
        acquisition.error =
            Amg8833DriverError::NotInitialized;

        last_error_ =
            acquisition.error;

        return acquisition;
    }

    last_status_ =
        Amg8833Status{};

    std::uint8_t status_register = 0;

    if (!bus_.readRegisters(
            Amg8833Registers::STATUS,
            &status_register,
            1))
    {
        acquisition.error =
            Amg8833DriverError::StatusReadFailed;

        last_error_ =
            acquisition.error;

        recordAcquisitionFailure(
            acquisition);

        return acquisition;
    }

    acquisition.status =
        Amg8833Registers::decodeStatus(
            status_register);

    last_status_ =
        acquisition.status;

    std::array<std::uint8_t, 2>
        thermistor_bytes{};

    if (!bus_.readRegisters(
            Amg8833Registers::THERMISTOR_LOW,
            thermistor_bytes.data(),
            thermistor_bytes.size()))
    {
        acquisition.error =
            Amg8833DriverError::
                ThermistorReadFailed;

        last_error_ =
            acquisition.error;

        recordAcquisitionFailure(
            acquisition);

        return acquisition;
    }

    Amg8833Decoder::PixelBytes
        pixel_bytes{};

    if (!bus_.readRegisters(
            Amg8833Registers::
                PIXEL_TEMPERATURE_START,
            pixel_bytes.data(),
            pixel_bytes.size()))
    {
        acquisition.error =
            Amg8833DriverError::PixelReadFailed;

        last_error_ =
            acquisition.error;

        recordAcquisitionFailure(
            acquisition);

        return acquisition;
    }

    ++frame_count_;

    const bool sensor_data_valid =
        !acquisition.status
             .pixelTemperatureOverflow &&
        !acquisition.status
             .thermistorOverflow;

    acquisition.frame =
        processor_.process(
            pixel_bytes,
            thermistor_bytes[0],
            thermistor_bytes[1],
            frame_count_,
            timestamp_ms,
            sensor_data_valid);

    acquisition.error =
        Amg8833DriverError::None;

    last_error_ =
        Amg8833DriverError::None;

    recordAcquisitionSuccess();

    return acquisition;
}

Amg8833InterruptMapResult
Amg8833Driver::readInterruptMap()
{
    Amg8833InterruptMapResult result;

    if (!initialized_)
    {
        result.error =
            Amg8833DriverError::NotInitialized;

        last_error_ =
            result.error;

        return result;
    }

    Amg8833InterruptMap::RawBytes
        raw_bytes{};

    if (!bus_.readRegisters(
            Amg8833Registers::
                INTERRUPT_TABLE_START,
            raw_bytes.data(),
            raw_bytes.size()))
    {
        result.error =
            Amg8833DriverError::
                InterruptTableReadFailed;

        last_error_ =
            result.error;

        recordInterruptMapFailure(
            result);

        return result;
    }

    result.map =
        Amg8833InterruptMap(
            raw_bytes);

    result.error =
        Amg8833DriverError::None;

    last_error_ =
        Amg8833DriverError::None;

    recordInterruptMapSuccess();

    return result;
}

bool Amg8833Driver::clearStatus()
{
    if (!initialized_)
    {
        last_error_ =
            Amg8833DriverError::NotInitialized;

        return false;
    }

    const std::uint8_t clear_value =
        Amg8833Registers::encodeStatusClear(
            last_status_);

    if (clear_value == 0U)
    {
        last_error_ =
            Amg8833DriverError::None;

        return true;
    }

    if (!bus_.writeRegister(
            Amg8833Registers::STATUS_CLEAR,
            clear_value))
    {
        last_error_ =
            Amg8833DriverError::
                StatusClearWriteFailed;

        return false;
    }

    last_status_ =
        Amg8833Status{};

    last_error_ =
        Amg8833DriverError::None;

    return true;
}

bool Amg8833Driver::validInterruptThresholds(
    const Amg8833InterruptThresholds& thresholds)
{
    if (!std::isfinite(
            thresholds.upper_temperature) ||
        !std::isfinite(
            thresholds.lower_temperature) ||
        !std::isfinite(
            thresholds.hysteresis))
    {
        return false;
    }

    if (thresholds.upper_temperature <
            MINIMUM_INTERRUPT_TEMPERATURE ||
        thresholds.upper_temperature >
            MAXIMUM_INTERRUPT_TEMPERATURE)
    {
        return false;
    }

    if (thresholds.lower_temperature <
            MINIMUM_INTERRUPT_TEMPERATURE ||
        thresholds.lower_temperature >
            MAXIMUM_INTERRUPT_TEMPERATURE)
    {
        return false;
    }

    if (thresholds.lower_temperature >
        thresholds.upper_temperature)
    {
        return false;
    }

    if (thresholds.hysteresis < 0.0f ||
        thresholds.hysteresis >
            MAXIMUM_INTERRUPT_TEMPERATURE)
    {
        return false;
    }

    const float threshold_distance =
        thresholds.upper_temperature -
        thresholds.lower_temperature;

    return thresholds.hysteresis <=
           threshold_distance;
}

std::uint16_t
Amg8833Driver::encodeInterruptTemperature(
    float temperature)
{
    float clamped_temperature =
        temperature;

    if (!std::isfinite(
            clamped_temperature))
    {
        clamped_temperature = 0.0f;
    }

    if (clamped_temperature <
        MINIMUM_INTERRUPT_TEMPERATURE)
    {
        clamped_temperature =
            MINIMUM_INTERRUPT_TEMPERATURE;
    }

    if (clamped_temperature >
        MAXIMUM_INTERRUPT_TEMPERATURE)
    {
        clamped_temperature =
            MAXIMUM_INTERRUPT_TEMPERATURE;
    }

    std::int32_t counts =
        static_cast<std::int32_t>(
            std::round(
                clamped_temperature /
                INTERRUPT_RESOLUTION));

    if (counts <
        MINIMUM_INTERRUPT_COUNTS)
    {
        counts =
            MINIMUM_INTERRUPT_COUNTS;
    }

    if (counts >
        MAXIMUM_INTERRUPT_COUNTS)
    {
        counts =
            MAXIMUM_INTERRUPT_COUNTS;
    }

    return static_cast<std::uint16_t>(
               counts) &
           0x0FFFU;
}

bool Amg8833Driver::performInitialization(
    bool preserve_health_counters)
{
    const std::uint32_t saved_total_failures =
        total_failures_;

    const std::uint32_t saved_recovery_attempts =
        recovery_attempts_;

    const std::uint32_t saved_successful_recoveries =
        successful_recoveries_;

    const std::uint32_t saved_failed_recoveries =
        failed_recoveries_;

    initialized_ = false;

    last_error_ =
        Amg8833DriverError::None;

    last_status_ =
        Amg8833Status{};

    frame_count_ = 0;

    consecutive_failures_ = 0;

    processor_.reset();

    if (!preserve_health_counters)
    {
        total_failures_ = 0;

        recovery_attempts_ = 0;

        successful_recoveries_ = 0;

        failed_recoveries_ = 0;
    }
    else
    {
        total_failures_ =
            saved_total_failures;

        recovery_attempts_ =
            saved_recovery_attempts;

        successful_recoveries_ =
            saved_successful_recoveries;

        failed_recoveries_ =
            saved_failed_recoveries;
    }

    if (config_.interrupt_thresholds.enabled &&
        !validInterruptThresholds(
            config_.interrupt_thresholds))
    {
        last_error_ =
            Amg8833DriverError::
                InvalidInterruptThresholds;

        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::POWER_CONTROL,
            Amg8833Registers::encodePowerMode(
                Amg8833PowerMode::Normal),
            Amg8833DriverError::
                PowerControlWriteFailed))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::RESET,
            Amg8833Registers::encodeResetCommand(
                Amg8833ResetCommand::InitialReset),
            Amg8833DriverError::
                InitialResetWriteFailed))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::FRAME_RATE,
            Amg8833Registers::encodeFrameRate(
                config_.frame_rate),
            Amg8833DriverError::
                FrameRateWriteFailed))
    {
        return false;
    }

    if (!configureMovingAverage())
    {
        return false;
    }

    if (!configureInterruptThresholds())
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::INTERRUPT_CONTROL,
            Amg8833Registers::
                encodeInterruptControl(
                    config_.interrupt),
            Amg8833DriverError::
                InterruptControlWriteFailed))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::STATUS_CLEAR,
            Amg8833Registers::STATUS_CLEAR_ALL,
            Amg8833DriverError::
                StatusClearWriteFailed))
    {
        return false;
    }

    initialized_ = true;

    last_error_ =
        Amg8833DriverError::None;

    return true;
}

bool Amg8833Driver::writeInitializationRegister(
    std::uint8_t register_address,
    std::uint8_t value,
    Amg8833DriverError error)
{
    if (bus_.writeRegister(
            register_address,
            value))
    {
        return true;
    }

    initialized_ = false;

    last_error_ = error;

    return false;
}

bool Amg8833Driver::configureMovingAverage()
{
    const Amg8833DriverError error =
        Amg8833DriverError::
            MovingAverageWriteFailed;

    if (!writeInitializationRegister(
            Amg8833Registers::
                RESERVED_AVERAGE_CONTROL,
            MOVING_AVERAGE_UNLOCK_VALUE_1,
            error))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::
                RESERVED_AVERAGE_CONTROL,
            MOVING_AVERAGE_UNLOCK_VALUE_2,
            error))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::
                RESERVED_AVERAGE_CONTROL,
            MOVING_AVERAGE_UNLOCK_VALUE_3,
            error))
    {
        return false;
    }

    if (!writeInitializationRegister(
            Amg8833Registers::AVERAGE,
            Amg8833Registers::
                encodeMovingAverage(
                    config_
                        .moving_average_enabled),
            error))
    {
        return false;
    }

    return writeInitializationRegister(
        Amg8833Registers::
            RESERVED_AVERAGE_CONTROL,
        MOVING_AVERAGE_LOCK_VALUE,
        error);
}

bool Amg8833Driver::configureInterruptThresholds()
{
    if (!config_.interrupt_thresholds.enabled)
    {
        return true;
    }

    if (!writeInterruptTemperature(
            Amg8833Registers::
                INTERRUPT_UPPER_LEVEL_LOW,
            Amg8833Registers::
                INTERRUPT_UPPER_LEVEL_HIGH,
            config_.interrupt_thresholds
                .upper_temperature))
    {
        return false;
    }

    if (!writeInterruptTemperature(
            Amg8833Registers::
                INTERRUPT_LOWER_LEVEL_LOW,
            Amg8833Registers::
                INTERRUPT_LOWER_LEVEL_HIGH,
            config_.interrupt_thresholds
                .lower_temperature))
    {
        return false;
    }

    return writeInterruptTemperature(
        Amg8833Registers::
            INTERRUPT_HYSTERESIS_LOW,
        Amg8833Registers::
            INTERRUPT_HYSTERESIS_HIGH,
        config_.interrupt_thresholds
            .hysteresis);
}

bool Amg8833Driver::writeInterruptTemperature(
    std::uint8_t low_register,
    std::uint8_t high_register,
    float temperature)
{
    const std::uint16_t encoded =
        encodeInterruptTemperature(
            temperature);

    const std::uint8_t low_byte =
        static_cast<std::uint8_t>(
            encoded & 0x00FFU);

    const std::uint8_t high_byte =
        static_cast<std::uint8_t>(
            (encoded >> 8U) &
            0x000FU);

    const Amg8833DriverError error =
        Amg8833DriverError::
            InterruptThresholdWriteFailed;

    if (!writeInitializationRegister(
            low_register,
            low_byte,
            error))
    {
        return false;
    }

    return writeInitializationRegister(
        high_register,
        high_byte,
        error);
}

void Amg8833Driver::recordAcquisitionSuccess()
{
    consecutive_failures_ = 0;
}

void Amg8833Driver::recordAcquisitionFailure(
    Amg8833Acquisition& acquisition)
{
    ++consecutive_failures_;
    ++total_failures_;

    if (!config_.recovery.enabled ||
        consecutive_failures_ <
            recoveryThreshold())
    {
        return;
    }

    acquisition.recovery_attempted = true;

    ++recovery_attempts_;

    const Amg8833DriverError acquisition_error =
        acquisition.error;

    const bool recovered =
        performInitialization(
            true);

    acquisition.recovery_succeeded =
        recovered;

    if (recovered)
    {
        ++successful_recoveries_;

        consecutive_failures_ = 0;

        last_error_ =
            acquisition_error;
    }
    else
    {
        ++failed_recoveries_;
    }
}

void Amg8833Driver::recordInterruptMapSuccess()
{
    consecutive_failures_ = 0;
}

void Amg8833Driver::recordInterruptMapFailure(
    Amg8833InterruptMapResult& result)
{
    ++consecutive_failures_;
    ++total_failures_;

    if (!config_.recovery.enabled ||
        consecutive_failures_ <
            recoveryThreshold())
    {
        return;
    }

    result.recovery_attempted = true;

    ++recovery_attempts_;

    const Amg8833DriverError read_error =
        result.error;

    const bool recovered =
        performInitialization(
            true);

    result.recovery_succeeded =
        recovered;

    if (recovered)
    {
        ++successful_recoveries_;

        consecutive_failures_ = 0;

        last_error_ =
            read_error;
    }
    else
    {
        ++failed_recoveries_;
    }
}

std::uint32_t
Amg8833Driver::recoveryThreshold() const
{
    if (config_.recovery.failure_threshold <
        1U)
    {
        return 1U;
    }

    return config_.recovery.failure_threshold;
}

}  // namespace leafsense::drivers