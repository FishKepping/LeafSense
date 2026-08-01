#pragma once

#include <cstddef>
#include <cstdint>

namespace leafsense::drivers {

/**
 * @brief Platform-independent register bus for the AMG8833.
 *
 * Platform adapters implement this interface using ESPHome, Arduino,
 * ESP-IDF, Linux I2C, a simulator, or another transport.
 *
 * The bus is expected to already be associated with one AMG8833 device
 * and its configured I2C address.
 *
 * Implementations must support reading the full 128-byte pixel register
 * range beginning at register 0x80.
 *
 * The interface performs no heap allocation.
 */
class Amg8833Bus
{
public:
    virtual ~Amg8833Bus() = default;

    /**
     * @brief Write one byte to one sensor register.
     *
     * @return true when the complete operation succeeds.
     */
    virtual bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) = 0;

    /**
     * @brief Read consecutive bytes beginning at a sensor register.
     *
     * @param start_register First register to read.
     * @param destination Destination byte buffer.
     * @param length Number of bytes to read.
     *
     * @return true when the complete operation succeeds.
     */
    virtual bool readRegisters(
        std::uint8_t start_register,
        std::uint8_t* destination,
        std::size_t length) = 0;
};

}  // namespace leafsense::drivers