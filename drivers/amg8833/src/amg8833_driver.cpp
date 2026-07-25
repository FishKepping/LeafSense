#include "leafsense/drivers/amg8833_driver.h"

#include <array>
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

}  // namespace

bool Amg8833Acquisition::success() const
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
      frame_count_(0)
{
}

const Amg8833DriverConfig&
Amg8833Driver::config() const
{
    return config_;
}

bool Amg8833Driver::initialize()
{
    reset();

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

    initialized_ =
        true;

    last_error_ =
        Amg8833DriverError::None;

    return true;
}

bool Amg8833Driver::initialized() const
{
    return initialized_;
}

void Amg8833Driver::reset()
{
    initialized_ =
        false;

    last_error_ =
        Amg8833DriverError::None;

    last_status_ =
        Amg8833Status{};

    frame_count_ =
        0;

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

    std::uint8_t status_register =
        0;

    if (!bus_.readRegisters(
            Amg8833Registers::STATUS,
            &status_register,
            1))
    {
        acquisition.error =
            Amg8833DriverError::StatusReadFailed;

        last_error_ =
            acquisition.error;

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

    return acquisition;
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

    initialized_ =
        false;

    last_error_ =
        error;

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

    if (!writeInitializationRegister(
            Amg8833Registers::
                RESERVED_AVERAGE_CONTROL,
            MOVING_AVERAGE_LOCK_VALUE,
            error))
    {
        return false;
    }

    return true;
}

}  // namespace leafsense::drivers