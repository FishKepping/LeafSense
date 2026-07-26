#include "esphome_bus.h"

namespace esphome {
namespace leafsense_amg8833 {

EspHomeAmg8833Bus::EspHomeAmg8833Bus(
    i2c::I2CDevice& device)
    : device_(device)
{
}

bool EspHomeAmg8833Bus::writeRegister(
    std::uint8_t register_address,
    std::uint8_t value)
{
    return device_.write_register(
               register_address,
               &value,
               sizeof(value)) ==
           i2c::ERROR_OK;
}

bool EspHomeAmg8833Bus::readRegisters(
    std::uint8_t start_register,
    std::uint8_t* destination,
    std::size_t length)
{
    if (destination == nullptr || length == 0U)
    {
        return false;
    }

    return device_.read_register(
               start_register,
               destination,
               length) ==
           i2c::ERROR_OK;
}

}  // namespace leafsense_amg8833
}  // namespace esphome
