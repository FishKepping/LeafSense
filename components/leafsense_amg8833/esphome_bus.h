#pragma once

#include <cstddef>
#include <cstdint>

#include "esphome/components/i2c/i2c.h"
#include "leafsense/drivers/amg8833_bus.h"

namespace esphome {
namespace leafsense_amg8833 {

class EspHomeAmg8833Bus final
    : public leafsense::drivers::Amg8833Bus
{
public:
    explicit EspHomeAmg8833Bus(
        i2c::I2CDevice& device);

    bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) override;

    bool readRegisters(
        std::uint8_t start_register,
        std::uint8_t* destination,
        std::size_t length) override;

private:
    i2c::I2CDevice& device_;
};

}  // namespace leafsense_amg8833
}  // namespace esphome
