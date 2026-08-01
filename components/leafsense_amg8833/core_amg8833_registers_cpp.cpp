#include "amg8833_registers.h"

#include <cstdint>

namespace leafsense {

bool Amg8833Status::clear() const
{
    return !any();
}

bool Amg8833Status::any() const
{
    return interrupt ||
           pixelTemperatureOverflow ||
           thermistorOverflow;
}

std::uint8_t Amg8833Registers::encodePowerMode(
    Amg8833PowerMode mode)
{
    return static_cast<std::uint8_t>(
        mode);
}

std::uint8_t Amg8833Registers::encodeResetCommand(
    Amg8833ResetCommand command)
{
    return static_cast<std::uint8_t>(
        command);
}

std::uint8_t Amg8833Registers::encodeFrameRate(
    Amg8833FrameRate frame_rate)
{
    return static_cast<std::uint8_t>(
        frame_rate);
}

std::uint8_t Amg8833Registers::encodeInterruptControl(
    const Amg8833InterruptConfig& config)
{
    std::uint8_t value =
        0x00;

    if (config.enabled)
    {
        value |=
            INTERRUPT_ENABLE_MASK;
    }

    if (config.mode ==
        Amg8833InterruptMode::Absolute)
    {
        value |=
            INTERRUPT_MODE_MASK;
    }

    return value;
}

Amg8833Status Amg8833Registers::decodeStatus(
    std::uint8_t register_value)
{
    Amg8833Status status;

    status.interrupt =
        (register_value &
         STATUS_INTERRUPT_MASK) != 0U;

    status.pixelTemperatureOverflow =
        (register_value &
         STATUS_PIXEL_OVERFLOW_MASK) != 0U;

    status.thermistorOverflow =
        (register_value &
         STATUS_THERMISTOR_OVERFLOW_MASK) != 0U;

    return status;
}

std::uint8_t Amg8833Registers::encodeStatusClear(
    const Amg8833Status& flags)
{
    std::uint8_t value =
        0x00;

    if (flags.interrupt)
    {
        value |=
            STATUS_CLEAR_INTERRUPT;
    }

    if (flags.pixelTemperatureOverflow)
    {
        value |=
            STATUS_CLEAR_PIXEL_OVERFLOW;
    }

    if (flags.thermistorOverflow)
    {
        value |=
            STATUS_CLEAR_THERMISTOR_OVERFLOW;
    }

    return value;
}

std::uint8_t Amg8833Registers::encodeMovingAverage(
    bool enabled)
{
    return enabled
               ? MOVING_AVERAGE_ENABLED
               : MOVING_AVERAGE_DISABLED;
}

std::uint8_t Amg8833Registers::pixelLowRegister(
    std::uint8_t pixel_index)
{
    if (pixel_index >= 64U)
    {
        return PIXEL_TEMPERATURE_END;
    }

    return static_cast<std::uint8_t>(
        PIXEL_TEMPERATURE_START +
        static_cast<std::uint8_t>(
            pixel_index * 2U));
}

std::uint8_t Amg8833Registers::pixelHighRegister(
    std::uint8_t pixel_index)
{
    if (pixel_index >= 64U)
    {
        return PIXEL_TEMPERATURE_END;
    }

    return static_cast<std::uint8_t>(
        pixelLowRegister(pixel_index) +
        1U);
}

}  // namespace leafsense