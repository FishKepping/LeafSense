#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "leafsense/amg8833_registers.h"

namespace {

using leafsense::Amg8833FrameRate;
using leafsense::Amg8833InterruptConfig;
using leafsense::Amg8833InterruptMode;
using leafsense::Amg8833PowerMode;
using leafsense::Amg8833Registers;
using leafsense::Amg8833ResetCommand;
using leafsense::Amg8833Status;

}  // namespace

TEST_CASE(
    "Amg8833Registers defines both supported I2C addresses",
    "[amg8833_registers][address]")
{
    REQUIRE(
        Amg8833Registers::DEFAULT_I2C_ADDRESS ==
        0x68);

    REQUIRE(
        Amg8833Registers::ALTERNATE_I2C_ADDRESS ==
        0x69);
}

TEST_CASE(
    "Amg8833Registers defines configuration register addresses",
    "[amg8833_registers][address]")
{
    REQUIRE(
        Amg8833Registers::POWER_CONTROL ==
        0x00);

    REQUIRE(
        Amg8833Registers::RESET ==
        0x01);

    REQUIRE(
        Amg8833Registers::FRAME_RATE ==
        0x02);

    REQUIRE(
        Amg8833Registers::INTERRUPT_CONTROL ==
        0x03);

    REQUIRE(
        Amg8833Registers::STATUS ==
        0x04);

    REQUIRE(
        Amg8833Registers::STATUS_CLEAR ==
        0x05);

    REQUIRE(
        Amg8833Registers::AVERAGE ==
        0x07);
}

TEST_CASE(
    "Amg8833Registers defines threshold register addresses",
    "[amg8833_registers][address]")
{
    REQUIRE(
        Amg8833Registers::INTERRUPT_UPPER_LEVEL_LOW ==
        0x08);

    REQUIRE(
        Amg8833Registers::INTERRUPT_UPPER_LEVEL_HIGH ==
        0x09);

    REQUIRE(
        Amg8833Registers::INTERRUPT_LOWER_LEVEL_LOW ==
        0x0A);

    REQUIRE(
        Amg8833Registers::INTERRUPT_LOWER_LEVEL_HIGH ==
        0x0B);

    REQUIRE(
        Amg8833Registers::INTERRUPT_HYSTERESIS_LOW ==
        0x0C);

    REQUIRE(
        Amg8833Registers::INTERRUPT_HYSTERESIS_HIGH ==
        0x0D);
}

TEST_CASE(
    "Amg8833Registers defines thermistor register addresses",
    "[amg8833_registers][address]")
{
    REQUIRE(
        Amg8833Registers::THERMISTOR_LOW ==
        0x0E);

    REQUIRE(
        Amg8833Registers::THERMISTOR_HIGH ==
        0x0F);
}

TEST_CASE(
    "Amg8833Registers defines pixel register range",
    "[amg8833_registers][address]")
{
    REQUIRE(
        Amg8833Registers::PIXEL_TEMPERATURE_START ==
        0x80);

    REQUIRE(
        Amg8833Registers::PIXEL_TEMPERATURE_END ==
        0xFF);

    REQUIRE(
        Amg8833Registers::PIXEL_BYTE_COUNT ==
        128);
}

TEST_CASE(
    "Amg8833Registers encodes normal power mode",
    "[amg8833_registers][power]")
{
    REQUIRE(
        Amg8833Registers::encodePowerMode(
            Amg8833PowerMode::Normal) ==
        0x00);
}

TEST_CASE(
    "Amg8833Registers encodes sleep power mode",
    "[amg8833_registers][power]")
{
    REQUIRE(
        Amg8833Registers::encodePowerMode(
            Amg8833PowerMode::Sleep) ==
        0x10);
}

TEST_CASE(
    "Amg8833Registers encodes standby power modes",
    "[amg8833_registers][power]")
{
    REQUIRE(
        Amg8833Registers::encodePowerMode(
            Amg8833PowerMode::Standby60Seconds) ==
        0x20);

    REQUIRE(
        Amg8833Registers::encodePowerMode(
            Amg8833PowerMode::Standby10Seconds) ==
        0x21);
}

TEST_CASE(
    "Amg8833Registers encodes reset commands",
    "[amg8833_registers][reset]")
{
    REQUIRE(
        Amg8833Registers::encodeResetCommand(
            Amg8833ResetCommand::FlagReset) ==
        0x30);

    REQUIRE(
        Amg8833Registers::encodeResetCommand(
            Amg8833ResetCommand::InitialReset) ==
        0x3F);
}

TEST_CASE(
    "Amg8833Registers encodes ten frames per second",
    "[amg8833_registers][frame_rate]")
{
    REQUIRE(
        Amg8833Registers::encodeFrameRate(
            Amg8833FrameRate::FramesPerSecond10) ==
        0x00);
}

TEST_CASE(
    "Amg8833Registers encodes one frame per second",
    "[amg8833_registers][frame_rate]")
{
    REQUIRE(
        Amg8833Registers::encodeFrameRate(
            Amg8833FrameRate::FramesPerSecond1) ==
        0x01);
}

TEST_CASE(
    "Amg8833Registers disables interrupts by default",
    "[amg8833_registers][interrupt]")
{
    const Amg8833InterruptConfig config;

    REQUIRE_FALSE(config.enabled);

    REQUIRE(
        config.mode ==
        Amg8833InterruptMode::Difference);

    REQUIRE(
        Amg8833Registers::encodeInterruptControl(
            config) ==
        0x00);
}

TEST_CASE(
    "Amg8833Registers enables difference-mode interrupt",
    "[amg8833_registers][interrupt]")
{
    Amg8833InterruptConfig config;

    config.enabled =
        true;

    config.mode =
        Amg8833InterruptMode::Difference;

    REQUIRE(
        Amg8833Registers::encodeInterruptControl(
            config) ==
        0x01);
}

TEST_CASE(
    "Amg8833Registers encodes disabled absolute mode",
    "[amg8833_registers][interrupt]")
{
    Amg8833InterruptConfig config;

    config.enabled =
        false;

    config.mode =
        Amg8833InterruptMode::Absolute;

    REQUIRE(
        Amg8833Registers::encodeInterruptControl(
            config) ==
        0x02);
}

TEST_CASE(
    "Amg8833Registers enables absolute-mode interrupt",
    "[amg8833_registers][interrupt]")
{
    Amg8833InterruptConfig config;

    config.enabled =
        true;

    config.mode =
        Amg8833InterruptMode::Absolute;

    REQUIRE(
        Amg8833Registers::encodeInterruptControl(
            config) ==
        0x03);
}

TEST_CASE(
    "Amg8833Registers decodes a clear status register",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0x00);

    REQUIRE_FALSE(status.interrupt);

    REQUIRE_FALSE(
        status.pixelTemperatureOverflow);

    REQUIRE_FALSE(
        status.thermistorOverflow);

    REQUIRE(status.clear());
    REQUIRE_FALSE(status.any());
}

TEST_CASE(
    "Amg8833Registers decodes interrupt status",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0x02);

    REQUIRE(status.interrupt);

    REQUIRE_FALSE(
        status.pixelTemperatureOverflow);

    REQUIRE_FALSE(
        status.thermistorOverflow);

    REQUIRE_FALSE(status.clear());
    REQUIRE(status.any());
}

TEST_CASE(
    "Amg8833Registers decodes pixel overflow status",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0x04);

    REQUIRE_FALSE(status.interrupt);

    REQUIRE(
        status.pixelTemperatureOverflow);

    REQUIRE_FALSE(
        status.thermistorOverflow);
}

TEST_CASE(
    "Amg8833Registers decodes thermistor overflow status",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0x08);

    REQUIRE_FALSE(status.interrupt);

    REQUIRE_FALSE(
        status.pixelTemperatureOverflow);

    REQUIRE(
        status.thermistorOverflow);
}

TEST_CASE(
    "Amg8833Registers decodes all status flags",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0x0E);

    REQUIRE(status.interrupt);

    REQUIRE(
        status.pixelTemperatureOverflow);

    REQUIRE(
        status.thermistorOverflow);

    REQUIRE(status.any());
}

TEST_CASE(
    "Amg8833Registers ignores unknown status bits",
    "[amg8833_registers][status]")
{
    const Amg8833Status status =
        Amg8833Registers::decodeStatus(
            0xF1);

    REQUIRE(status.clear());
}

TEST_CASE(
    "Amg8833Registers encodes no status-clear flags",
    "[amg8833_registers][status]")
{
    const Amg8833Status flags;

    REQUIRE(
        Amg8833Registers::encodeStatusClear(
            flags) ==
        0x00);
}

TEST_CASE(
    "Amg8833Registers encodes individual status-clear flags",
    "[amg8833_registers][status]")
{
    Amg8833Status interrupt;
    interrupt.interrupt =
        true;

    REQUIRE(
        Amg8833Registers::encodeStatusClear(
            interrupt) ==
        0x02);

    Amg8833Status pixel_overflow;
    pixel_overflow.pixelTemperatureOverflow =
        true;

    REQUIRE(
        Amg8833Registers::encodeStatusClear(
            pixel_overflow) ==
        0x04);

    Amg8833Status thermistor_overflow;
    thermistor_overflow.thermistorOverflow =
        true;

    REQUIRE(
        Amg8833Registers::encodeStatusClear(
            thermistor_overflow) ==
        0x08);
}

TEST_CASE(
    "Amg8833Registers encodes all status-clear flags",
    "[amg8833_registers][status]")
{
    Amg8833Status flags;

    flags.interrupt =
        true;

    flags.pixelTemperatureOverflow =
        true;

    flags.thermistorOverflow =
        true;

    REQUIRE(
        Amg8833Registers::encodeStatusClear(
            flags) ==
        Amg8833Registers::STATUS_CLEAR_ALL);

    REQUIRE(
        Amg8833Registers::STATUS_CLEAR_ALL ==
        0x0E);
}

TEST_CASE(
    "Amg8833Registers encodes moving-average mode",
    "[amg8833_registers][average]")
{
    REQUIRE(
        Amg8833Registers::encodeMovingAverage(
            false) ==
        0x00);

    REQUIRE(
        Amg8833Registers::encodeMovingAverage(
            true) ==
        0x20);
}

TEST_CASE(
    "Amg8833Registers maps the first pixel registers",
    "[amg8833_registers][pixel]")
{
    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            0) ==
        0x80);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            0) ==
        0x81);
}

TEST_CASE(
    "Amg8833Registers maps row boundary pixel registers",
    "[amg8833_registers][pixel]")
{
    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            7) ==
        0x8E);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            7) ==
        0x8F);

    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            8) ==
        0x90);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            8) ==
        0x91);
}

TEST_CASE(
    "Amg8833Registers maps the final pixel registers",
    "[amg8833_registers][pixel]")
{
    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            63) ==
        0xFE);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            63) ==
        0xFF);
}

TEST_CASE(
    "Amg8833Registers rejects invalid pixel indices",
    "[amg8833_registers][pixel]")
{
    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            64) ==
        Amg8833Registers::PIXEL_TEMPERATURE_END);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            64) ==
        Amg8833Registers::PIXEL_TEMPERATURE_END);

    REQUIRE(
        Amg8833Registers::pixelLowRegister(
            255) ==
        Amg8833Registers::PIXEL_TEMPERATURE_END);

    REQUIRE(
        Amg8833Registers::pixelHighRegister(
            255) ==
        Amg8833Registers::PIXEL_TEMPERATURE_END);
}