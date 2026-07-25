#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/drivers/amg8833_driver.h"

namespace {

using leafsense::Amg8833Registers;

using leafsense::drivers::Amg8833Bus;
using leafsense::drivers::Amg8833Driver;
using leafsense::drivers::Amg8833DriverConfig;
using leafsense::drivers::Amg8833DriverError;
using leafsense::drivers::Amg8833InterruptMap;
using leafsense::drivers::Amg8833InterruptMapResult;
using leafsense::drivers::Amg8833InterruptThresholds;

struct WriteOperation
{
    std::uint8_t register_address;
    std::uint8_t value;
};

class InterruptTestBus final
    : public Amg8833Bus
{
public:
    static constexpr std::size_t NO_FAILURE =
        std::numeric_limits<std::size_t>::max();

    InterruptTestBus()
        : registers_(),
          writes_(),
          read_count_(0),
          fail_write_call_(NO_FAILURE),
          fail_read_(false)
    {
    }

    bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) override
    {
        const std::size_t call_index =
            writes_.size();

        writes_.push_back(
            WriteOperation{
                register_address,
                value});

        if (call_index ==
            fail_write_call_)
        {
            return false;
        }

        registers_[register_address] =
            value;

        return true;
    }

    bool readRegisters(
        std::uint8_t start_register,
        std::uint8_t* destination,
        std::size_t length) override
    {
        ++read_count_;

        if (fail_read_)
        {
            return false;
        }

        const std::size_t start =
            static_cast<std::size_t>(
                start_register);

        if (start + length >
            registers_.size())
        {
            return false;
        }

        for (std::size_t index = 0;
             index < length;
             ++index)
        {
            destination[index] =
                registers_[start + index];
        }

        return true;
    }

    void setInterruptByte(
        std::size_t byte_index,
        std::uint8_t value)
    {
        registers_[
            static_cast<std::size_t>(
                Amg8833Registers::
                    INTERRUPT_TABLE_START) +
            byte_index] =
                value;
    }

    void failWriteAt(
        std::size_t call_index)
    {
        fail_write_call_ =
            call_index;
    }

    void setFailRead(
        bool fail)
    {
        fail_read_ = fail;
    }

    const std::vector<WriteOperation>&
    writes() const
    {
        return writes_;
    }

    std::size_t readCount() const
    {
        return read_count_;
    }

private:
    std::array<std::uint8_t, 256>
        registers_;

    std::vector<WriteOperation>
        writes_;

    std::size_t read_count_;

    std::size_t fail_write_call_;

    bool fail_read_;
};

void requireWrite(
    const InterruptTestBus& bus,
    std::size_t index,
    std::uint8_t register_address,
    std::uint8_t value)
{
    REQUIRE(index < bus.writes().size());

    REQUIRE(
        bus.writes()[index]
            .register_address ==
        register_address);

    REQUIRE(
        bus.writes()[index].value ==
        value);
}

}  // namespace

TEST_CASE(
    "Interrupt thresholds are disabled by default",
    "[amg8833_driver][interrupt][configuration]")
{
    InterruptTestBus bus;

    const Amg8833Driver driver(
        bus);

    REQUIRE_FALSE(
        driver.config()
            .interrupt_thresholds
            .enabled);

    REQUIRE(
        driver.config()
            .interrupt_thresholds
            .upper_temperature ==
        30.0f);

    REQUIRE(
        driver.config()
            .interrupt_thresholds
            .lower_temperature ==
        10.0f);

    REQUIRE(
        driver.config()
            .interrupt_thresholds
            .hysteresis ==
        1.0f);
}

TEST_CASE(
    "Interrupt temperature encoder uses quarter-degree resolution",
    "[amg8833_driver][interrupt][encoding]")
{
    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                0.0f) ==
        0x000);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                0.25f) ==
        0x001);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                25.0f) ==
        0x064);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                30.0f) ==
        0x078);
}

TEST_CASE(
    "Interrupt temperature encoder supports negative values",
    "[amg8833_driver][interrupt][encoding]")
{
    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                -0.25f) ==
        0xFFF);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                -1.0f) ==
        0xFFC);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                -20.0f) ==
        0xFB0);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                -512.0f) ==
        0x800);
}

TEST_CASE(
    "Interrupt temperature encoder clamps the supported range",
    "[amg8833_driver][interrupt][encoding]")
{
    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                -1000.0f) ==
        0x800);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                1000.0f) ==
        0x7FF);

    REQUIRE(
        Amg8833Driver::
            encodeInterruptTemperature(
                511.75f) ==
        0x7FF);
}

TEST_CASE(
    "Interrupt threshold validation accepts a valid configuration",
    "[amg8833_driver][interrupt][validation]")
{
    Amg8833InterruptThresholds thresholds;

    thresholds.enabled = true;

    thresholds.upper_temperature =
        30.0f;

    thresholds.lower_temperature =
        10.0f;

    thresholds.hysteresis =
        2.0f;

    REQUIRE(
        Amg8833Driver::
            validInterruptThresholds(
                thresholds));
}

TEST_CASE(
    "Interrupt threshold validation rejects reversed limits",
    "[amg8833_driver][interrupt][validation]")
{
    Amg8833InterruptThresholds thresholds;

    thresholds.upper_temperature =
        10.0f;

    thresholds.lower_temperature =
        20.0f;

    REQUIRE_FALSE(
        Amg8833Driver::
            validInterruptThresholds(
                thresholds));
}

TEST_CASE(
    "Interrupt threshold validation rejects negative hysteresis",
    "[amg8833_driver][interrupt][validation]")
{
    Amg8833InterruptThresholds thresholds;

    thresholds.hysteresis =
        -0.25f;

    REQUIRE_FALSE(
        Amg8833Driver::
            validInterruptThresholds(
                thresholds));
}

TEST_CASE(
    "Interrupt threshold validation rejects excessive hysteresis",
    "[amg8833_driver][interrupt][validation]")
{
    Amg8833InterruptThresholds thresholds;

    thresholds.upper_temperature =
        20.0f;

    thresholds.lower_temperature =
        10.0f;

    thresholds.hysteresis =
        11.0f;

    REQUIRE_FALSE(
        Amg8833Driver::
            validInterruptThresholds(
                thresholds));
}

TEST_CASE(
    "Default initialization does not write threshold registers",
    "[amg8833_driver][interrupt][initialization]")
{
    InterruptTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    REQUIRE(
        bus.writes().size() ==
        10);
}

TEST_CASE(
    "Initialization writes configured interrupt thresholds",
    "[amg8833_driver][interrupt][initialization]")
{
    InterruptTestBus bus;

    Amg8833DriverConfig config;

    config.interrupt_thresholds.enabled =
        true;

    config.interrupt_thresholds
        .upper_temperature =
        30.0f;

    config.interrupt_thresholds
        .lower_temperature =
        -10.0f;

    config.interrupt_thresholds
        .hysteresis =
        2.0f;

    config.interrupt.enabled =
        true;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    REQUIRE(
        bus.writes().size() ==
        16);

    requireWrite(
        bus,
        8,
        Amg8833Registers::
            INTERRUPT_UPPER_LEVEL_LOW,
        0x78);

    requireWrite(
        bus,
        9,
        Amg8833Registers::
            INTERRUPT_UPPER_LEVEL_HIGH,
        0x00);

    requireWrite(
        bus,
        10,
        Amg8833Registers::
            INTERRUPT_LOWER_LEVEL_LOW,
        0xD8);

    requireWrite(
        bus,
        11,
        Amg8833Registers::
            INTERRUPT_LOWER_LEVEL_HIGH,
        0x0F);

    requireWrite(
        bus,
        12,
        Amg8833Registers::
            INTERRUPT_HYSTERESIS_LOW,
        0x08);

    requireWrite(
        bus,
        13,
        Amg8833Registers::
            INTERRUPT_HYSTERESIS_HIGH,
        0x00);

    requireWrite(
        bus,
        14,
        Amg8833Registers::
            INTERRUPT_CONTROL,
        0x01);

    requireWrite(
        bus,
        15,
        Amg8833Registers::
            STATUS_CLEAR,
        0x0E);
}

TEST_CASE(
    "Invalid enabled thresholds stop initialization before bus access",
    "[amg8833_driver][interrupt][initialization][error]")
{
    InterruptTestBus bus;

    Amg8833DriverConfig config;

    config.interrupt_thresholds.enabled =
        true;

    config.interrupt_thresholds
        .upper_temperature =
        10.0f;

    config.interrupt_thresholds
        .lower_temperature =
        20.0f;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE_FALSE(
        driver.initialize());

    REQUIRE_FALSE(
        driver.initialized());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::
            InvalidInterruptThresholds);

    REQUIRE(
        bus.writes().empty());
}

TEST_CASE(
    "Threshold-register write failures are reported",
    "[amg8833_driver][interrupt][initialization][error]")
{
    for (std::size_t failed_write = 8;
         failed_write <= 13;
         ++failed_write)
    {
        DYNAMIC_SECTION(
            "threshold write "
            << failed_write)
        {
            InterruptTestBus bus;

            bus.failWriteAt(
                failed_write);

            Amg8833DriverConfig config;

            config.interrupt_thresholds
                .enabled =
                true;

            Amg8833Driver driver(
                bus,
                config);

            REQUIRE_FALSE(
                driver.initialize());

            REQUIRE(
                driver.lastError() ==
                Amg8833DriverError::
                    InterruptThresholdWriteFailed);
        }
    }
}

TEST_CASE(
    "Empty interrupt map contains no active pixels",
    "[amg8833_driver][interrupt][map]")
{
    InterruptTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE(result.success());

    REQUIRE(result.map.empty());

    REQUIRE_FALSE(result.map.any());

    REQUIRE(
        result.map.activeCount() ==
        0);

    REQUIRE_FALSE(
        result.map.active(0, 0));

    REQUIRE_FALSE(
        result.map.active(63));
}

TEST_CASE(
    "Interrupt map decodes pixel bits",
    "[amg8833_driver][interrupt][map]")
{
    InterruptTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setInterruptByte(
        0,
        0b10000001);

    bus.setInterruptByte(
        1,
        0b00000001);

    bus.setInterruptByte(
        7,
        0b10000000);

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE(result.success());

    REQUIRE(result.map.any());

    REQUIRE_FALSE(result.map.empty());

    REQUIRE(
        result.map.activeCount() ==
        4);

    REQUIRE(
        result.map.active(0));

    REQUIRE(
        result.map.active(7));

    REQUIRE(
        result.map.active(8));

    REQUIRE(
        result.map.active(63));

    REQUIRE(
        result.map.active(0, 0));

    REQUIRE(
        result.map.active(0, 7));

    REQUIRE(
        result.map.active(1, 0));

    REQUIRE(
        result.map.active(7, 7));

    REQUIRE_FALSE(
        result.map.active(3, 3));
}

TEST_CASE(
    "Interrupt map rejects invalid coordinates",
    "[amg8833_driver][interrupt][map]")
{
    const Amg8833InterruptMap map;

    REQUIRE_FALSE(
        map.active(8, 0));

    REQUIRE_FALSE(
        map.active(0, 8));

    REQUIRE_FALSE(
        map.active(64));
}

TEST_CASE(
    "Interrupt map preserves raw bytes",
    "[amg8833_driver][interrupt][map]")
{
    InterruptTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setInterruptByte(0, 0x12);
    bus.setInterruptByte(1, 0x34);
    bus.setInterruptByte(2, 0x56);
    bus.setInterruptByte(3, 0x78);
    bus.setInterruptByte(4, 0x9A);
    bus.setInterruptByte(5, 0xBC);
    bus.setInterruptByte(6, 0xDE);
    bus.setInterruptByte(7, 0xF0);

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE(result.success());

    REQUIRE(
        result.map.rawBytes()[0] ==
        0x12);

    REQUIRE(
        result.map.rawBytes()[7] ==
        0xF0);
}

TEST_CASE(
    "Interrupt map cannot be read before initialization",
    "[amg8833_driver][interrupt][map][error]")
{
    InterruptTestBus bus;

    Amg8833Driver driver(
        bus);

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE_FALSE(result.success());

    REQUIRE(
        result.error ==
        Amg8833DriverError::
            NotInitialized);

    REQUIRE(
        bus.readCount() ==
        0);
}

TEST_CASE(
    "Interrupt map read failures update driver health",
    "[amg8833_driver][interrupt][map][error]")
{
    InterruptTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.setFailRead(true);

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE_FALSE(result.success());

    REQUIRE(
        result.error ==
        Amg8833DriverError::
            InterruptTableReadFailed);

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::
            InterruptTableReadFailed);

    REQUIRE(
        driver.health()
            .consecutive_failures ==
        1);

    REQUIRE(
        driver.health()
            .total_failures ==
        1);
}

TEST_CASE(
    "Interrupt map failures can trigger automatic recovery",
    "[amg8833_driver][interrupt][map][recovery]")
{
    InterruptTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        true;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.setFailRead(true);

    const Amg8833InterruptMapResult result =
        driver.readInterruptMap();

    REQUIRE_FALSE(result.success());

    REQUIRE(result.recovery_attempted);

    REQUIRE(result.recovery_succeeded);

    REQUIRE(driver.initialized());

    REQUIRE(
        driver.health()
            .successful_recoveries ==
        1);
}