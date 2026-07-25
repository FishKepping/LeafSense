#pragma once

#include <cstdint>

namespace leafsense {

/**
 * @brief AMG8833 operating power modes.
 *
 * Values correspond directly to the Power Control register.
 */
enum class Amg8833PowerMode : std::uint8_t
{
    Normal = 0x00,
    Sleep = 0x10,
    Standby60Seconds = 0x20,
    Standby10Seconds = 0x21
};

/**
 * @brief AMG8833 output frame rates.
 *
 * Values correspond directly to the Frame Rate register.
 */
enum class Amg8833FrameRate : std::uint8_t
{
    FramesPerSecond10 = 0x00,
    FramesPerSecond1 = 0x01
};

/**
 * @brief AMG8833 reset commands.
 *
 * InitialReset performs a complete sensor reset.
 *
 * FlagReset clears status and interrupt flags without performing a
 * complete device initialization.
 */
enum class Amg8833ResetCommand : std::uint8_t
{
    FlagReset = 0x30,
    InitialReset = 0x3F
};

/**
 * @brief AMG8833 interrupt comparison modes.
 */
enum class Amg8833InterruptMode : std::uint8_t
{
    Difference = 0x00,
    Absolute = 0x01
};

/**
 * @brief Configuration for the AMG8833 interrupt-control register.
 */
struct Amg8833InterruptConfig
{
    bool enabled = false;

    Amg8833InterruptMode mode =
        Amg8833InterruptMode::Difference;
};

/**
 * @brief Decoded AMG8833 status-register flags.
 */
struct Amg8833Status
{
    bool interrupt = false;
    bool pixelTemperatureOverflow = false;
    bool thermistorOverflow = false;

    /**
     * Return true when no status flags are active.
     */
    bool clear() const;

    /**
     * Return true when one or more status flags are active.
     */
    bool any() const;
};

/**
 * @brief AMG8833 register addresses and register-value helpers.
 *
 * This class contains no I2C implementation. It only defines the
 * register model used by platform adapters.
 *
 * All functions are stateless and perform no heap allocation.
 */
class Amg8833Registers
{
public:
    /**
     * Default AMG8833 I2C address when the address-select pin is low.
     */
    static constexpr std::uint8_t DEFAULT_I2C_ADDRESS =
        0x68;

    /**
     * Alternate AMG8833 I2C address when the address-select pin is high.
     */
    static constexpr std::uint8_t ALTERNATE_I2C_ADDRESS =
        0x69;

    static constexpr std::uint8_t POWER_CONTROL =
        0x00;

    static constexpr std::uint8_t RESET =
        0x01;

    static constexpr std::uint8_t FRAME_RATE =
        0x02;

    static constexpr std::uint8_t INTERRUPT_CONTROL =
        0x03;

    static constexpr std::uint8_t STATUS =
        0x04;

    static constexpr std::uint8_t STATUS_CLEAR =
        0x05;

    static constexpr std::uint8_t AVERAGE =
        0x07;

    static constexpr std::uint8_t INTERRUPT_UPPER_LEVEL_LOW =
        0x08;

    static constexpr std::uint8_t INTERRUPT_UPPER_LEVEL_HIGH =
        0x09;

    static constexpr std::uint8_t INTERRUPT_LOWER_LEVEL_LOW =
        0x0A;

    static constexpr std::uint8_t INTERRUPT_LOWER_LEVEL_HIGH =
        0x0B;

    static constexpr std::uint8_t INTERRUPT_HYSTERESIS_LOW =
        0x0C;

    static constexpr std::uint8_t INTERRUPT_HYSTERESIS_HIGH =
        0x0D;

    static constexpr std::uint8_t THERMISTOR_LOW =
        0x0E;

    static constexpr std::uint8_t THERMISTOR_HIGH =
        0x0F;

    static constexpr std::uint8_t INTERRUPT_TABLE_START =
        0x10;

    static constexpr std::uint8_t INTERRUPT_TABLE_END =
        0x17;

    static constexpr std::uint8_t RESERVED_AVERAGE_CONTROL =
        0x1F;

    static constexpr std::uint8_t PIXEL_TEMPERATURE_START =
        0x80;

    static constexpr std::uint8_t PIXEL_TEMPERATURE_END =
        0xFF;

    static constexpr std::uint8_t PIXEL_BYTE_COUNT =
        128;

    static constexpr std::uint8_t INTERRUPT_ENABLE_MASK =
        0x01;

    static constexpr std::uint8_t INTERRUPT_MODE_MASK =
        0x02;

    static constexpr std::uint8_t STATUS_INTERRUPT_MASK =
        0x02;

    static constexpr std::uint8_t STATUS_PIXEL_OVERFLOW_MASK =
        0x04;

    static constexpr std::uint8_t STATUS_THERMISTOR_OVERFLOW_MASK =
        0x08;

    static constexpr std::uint8_t STATUS_CLEAR_INTERRUPT =
        0x02;

    static constexpr std::uint8_t STATUS_CLEAR_PIXEL_OVERFLOW =
        0x04;

    static constexpr std::uint8_t STATUS_CLEAR_THERMISTOR_OVERFLOW =
        0x08;

    static constexpr std::uint8_t STATUS_CLEAR_ALL =
        STATUS_CLEAR_INTERRUPT |
        STATUS_CLEAR_PIXEL_OVERFLOW |
        STATUS_CLEAR_THERMISTOR_OVERFLOW;

    static constexpr std::uint8_t MOVING_AVERAGE_DISABLED =
        0x00;

    static constexpr std::uint8_t MOVING_AVERAGE_ENABLED =
        0x20;

    /**
     * Encode a power mode for the Power Control register.
     */
    static std::uint8_t encodePowerMode(
        Amg8833PowerMode mode);

    /**
     * Encode a reset command for the Reset register.
     */
    static std::uint8_t encodeResetCommand(
        Amg8833ResetCommand command);

    /**
     * Encode a frame rate for the Frame Rate register.
     */
    static std::uint8_t encodeFrameRate(
        Amg8833FrameRate frame_rate);

    /**
     * Encode the Interrupt Control register.
     *
     * Register bit layout:
     *
     *     bit 0: interrupt output enabled
     *     bit 1: interrupt mode
     */
    static std::uint8_t encodeInterruptControl(
        const Amg8833InterruptConfig& config);

    /**
     * Decode the sensor Status register.
     *
     * Unknown and reserved bits are ignored.
     */
    static Amg8833Status decodeStatus(
        std::uint8_t register_value);

    /**
     * Encode a value for the Status Clear register.
     */
    static std::uint8_t encodeStatusClear(
        const Amg8833Status& flags);

    /**
     * Encode the moving-average setting.
     */
    static std::uint8_t encodeMovingAverage(
        bool enabled);

    /**
     * Return the low-byte register for a pixel index.
     *
     * Pixel indices are expected to be in the range 0 through 63.
     * Values outside that range return PIXEL_TEMPERATURE_END.
     */
    static std::uint8_t pixelLowRegister(
        std::uint8_t pixel_index);

    /**
     * Return the high-byte register for a pixel index.
     *
     * Pixel indices are expected to be in the range 0 through 63.
     * Values outside that range return PIXEL_TEMPERATURE_END.
     */
    static std::uint8_t pixelHighRegister(
        std::uint8_t pixel_index);
};

}  // namespace leafsense