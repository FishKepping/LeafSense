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
using leafsense::drivers::Amg8833Acquisition;
using leafsense::drivers::Amg8833Bus;
using leafsense::drivers::Amg8833Driver;
using leafsense::drivers::Amg8833DriverConfig;
using leafsense::drivers::Amg8833DriverError;
using leafsense::drivers::Amg8833DriverHealth;

class RecoveryTestBus final
    : public Amg8833Bus
{
public:
    static constexpr std::size_t NO_FAILURE =
        std::numeric_limits<std::size_t>::max();

    RecoveryTestBus()
        : registers_(),
          write_count_(0),
          read_count_(0),
          failed_read_count_(0),
          fail_next_reads_(0),
          fail_writes_(false)
    {
    }

    bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) override
    {
        ++write_count_;

        if (fail_writes_)
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

        if (fail_next_reads_ > 0U)
        {
            --fail_next_reads_;

            ++failed_read_count_;

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

    void failNextReads(
        std::size_t count)
    {
        fail_next_reads_ =
            count;
    }

    void setFailWrites(
        bool fail)
    {
        fail_writes_ =
            fail;
    }

    void prepareValidFrame()
    {
        registers_[
            Amg8833Registers::STATUS] =
            0x00;

        registers_[
            Amg8833Registers::THERMISTOR_LOW] =
            0x90;

        registers_[
            Amg8833Registers::THERMISTOR_HIGH] =
            0x01;

        for (std::size_t pixel = 0;
             pixel < 64;
             ++pixel)
        {
            const std::size_t address =
                static_cast<std::size_t>(
                    Amg8833Registers::
                        PIXEL_TEMPERATURE_START) +
                pixel * 2;

            registers_[address] =
                0x64;

            registers_[address + 1] =
                0x00;
        }
    }

    std::size_t writeCount() const
    {
        return write_count_;
    }

    std::size_t readCount() const
    {
        return read_count_;
    }

    std::size_t failedReadCount() const
    {
        return failed_read_count_;
    }

private:
    std::array<std::uint8_t, 256>
        registers_;

    std::size_t write_count_;

    std::size_t read_count_;

    std::size_t failed_read_count_;

    std::size_t fail_next_reads_;

    bool fail_writes_;
};

}  // namespace

TEST_CASE(
    "AMG8833 recovery is enabled by default",
    "[amg8833_driver][recovery][configuration]")
{
    RecoveryTestBus bus;

    const Amg8833Driver driver(
        bus);

    REQUIRE(
        driver.config()
            .recovery.enabled);

    REQUIRE(
        driver.config()
            .recovery.failure_threshold ==
        3);
}

TEST_CASE(
    "Driver health is unhealthy before initialization",
    "[amg8833_driver][recovery][health]")
{
    RecoveryTestBus bus;

    const Amg8833Driver driver(
        bus);

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE_FALSE(health.initialized);
    REQUIRE_FALSE(health.healthy());

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(
        health.total_failures ==
        0);

    REQUIRE(
        health.recovery_attempts ==
        0);
}

TEST_CASE(
    "Driver health is healthy after initialization",
    "[amg8833_driver][recovery][health]")
{
    RecoveryTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(health.initialized);
    REQUIRE(health.healthy());
}

TEST_CASE(
    "One failed acquisition increments health counters",
    "[amg8833_driver][recovery][failure]")
{
    RecoveryTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(acquisition.success());

    REQUIRE(
        acquisition.error ==
        Amg8833DriverError::
            StatusReadFailed);

    REQUIRE_FALSE(
        acquisition.recovery_attempted);

    REQUIRE_FALSE(
        acquisition.recovery_succeeded);

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.consecutive_failures ==
        1);

    REQUIRE(
        health.total_failures ==
        1);

    REQUIRE(
        health.recovery_attempts ==
        0);

    REQUIRE_FALSE(health.healthy());
}

TEST_CASE(
    "Successful acquisition resets consecutive failure count",
    "[amg8833_driver][recovery][success]")
{
    RecoveryTestBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    REQUIRE_FALSE(
        driver.readFrame(1000)
            .success());

    REQUIRE(
        driver.health()
            .consecutive_failures ==
        1);

    bus.prepareValidFrame();

    REQUIRE(
        driver.readFrame(1100)
            .success());

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(
        health.total_failures ==
        1);

    REQUIRE(health.healthy());
}

TEST_CASE(
    "Driver recovers after configured failure threshold",
    "[amg8833_driver][recovery]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        true;

    config.recovery.failure_threshold =
        2;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    const std::size_t initialization_writes =
        bus.writeCount();

    bus.failNextReads(2);

    const Amg8833Acquisition first =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(first.success());

    REQUIRE_FALSE(
        first.recovery_attempted);

    const Amg8833Acquisition second =
        driver.readFrame(
            1100);

    REQUIRE_FALSE(second.success());

    REQUIRE(
        second.error ==
        Amg8833DriverError::
            StatusReadFailed);

    REQUIRE(
        second.recovery_attempted);

    REQUIRE(
        second.recovery_succeeded);

    REQUIRE(driver.initialized());

    REQUIRE(
        bus.writeCount() ==
        initialization_writes * 2);

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(
        health.total_failures ==
        2);

    REQUIRE(
        health.recovery_attempts ==
        1);

    REQUIRE(
        health.successful_recoveries ==
        1);

    REQUIRE(
        health.failed_recoveries ==
        0);
}

TEST_CASE(
    "Acquisition succeeds after automatic recovery",
    "[amg8833_driver][recovery][resume]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    const Amg8833Acquisition failed =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(failed.success());

    REQUIRE(
        failed.recovery_attempted);

    REQUIRE(
        failed.recovery_succeeded);

    bus.prepareValidFrame();

    const Amg8833Acquisition recovered =
        driver.readFrame(
            1100);

    REQUIRE(recovered.success());
    REQUIRE(recovered.frame.isValid());

    REQUIRE(
        recovered.frame.frameNumber() ==
        1);

    REQUIRE(
        driver.health()
            .consecutive_failures ==
        0);

    REQUIRE(
        driver.health()
            .successful_recoveries ==
        1);
}

TEST_CASE(
    "Automatic recovery can be disabled",
    "[amg8833_driver][recovery][disabled]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    const std::size_t writes_before =
        bus.writeCount();

    bus.failNextReads(3);

    for (std::size_t attempt = 0;
         attempt < 3;
         ++attempt)
    {
        const Amg8833Acquisition acquisition =
            driver.readFrame(
                static_cast<std::uint32_t>(
                    1000 + attempt));

        REQUIRE_FALSE(
            acquisition.success());

        REQUIRE_FALSE(
            acquisition.recovery_attempted);
    }

    REQUIRE(
        bus.writeCount() ==
        writes_before);

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.consecutive_failures ==
        3);

    REQUIRE(
        health.total_failures ==
        3);

    REQUIRE(
        health.recovery_attempts ==
        0);
}

TEST_CASE(
    "Zero recovery threshold is treated as one",
    "[amg8833_driver][recovery][configuration]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.failure_threshold =
        0;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(acquisition.success());

    REQUIRE(
        acquisition.recovery_attempted);

    REQUIRE(
        acquisition.recovery_succeeded);
}

TEST_CASE(
    "Failed automatic recovery is recorded",
    "[amg8833_driver][recovery][failure]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    bus.setFailWrites(true);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(acquisition.success());

    REQUIRE(
        acquisition.recovery_attempted);

    REQUIRE_FALSE(
        acquisition.recovery_succeeded);

    REQUIRE_FALSE(
        driver.initialized());

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE_FALSE(health.initialized);

    REQUIRE(
        health.total_failures ==
        1);

    REQUIRE(
        health.recovery_attempts ==
        1);

    REQUIRE(
        health.successful_recoveries ==
        0);

    REQUIRE(
        health.failed_recoveries ==
        1);

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::
            PowerControlWriteFailed);
}

TEST_CASE(
    "Different acquisition read errors count toward recovery",
    "[amg8833_driver][recovery][read_errors]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.prepareValidFrame();

    /*
     * Allow the status read and fail the thermistor read.
     */
    bus.failNextReads(0);

    class ThermistorFailureBus final
        : public Amg8833Bus
    {
    public:
        ThermistorFailureBus()
            : registers_(),
              reads_(0),
              writes_(0)
        {
        }

        bool writeRegister(
            std::uint8_t register_address,
            std::uint8_t value) override
        {
            ++writes_;

            registers_[register_address] =
                value;

            return true;
        }

        bool readRegisters(
            std::uint8_t start_register,
            std::uint8_t* destination,
            std::size_t length) override
        {
            ++reads_;

            if (start_register ==
                Amg8833Registers::
                    THERMISTOR_LOW)
            {
                return false;
            }

            for (std::size_t index = 0;
                 index < length;
                 ++index)
            {
                destination[index] =
                    registers_[
                        static_cast<std::size_t>(
                            start_register) +
                        index];
            }

            return true;
        }

    private:
        std::array<std::uint8_t, 256>
            registers_;

        std::size_t reads_;

        std::size_t writes_;
    };

    ThermistorFailureBus thermistor_bus;

    Amg8833Driver thermistor_driver(
        thermistor_bus,
        config);

    REQUIRE(
        thermistor_driver.initialize());

    const Amg8833Acquisition acquisition =
        thermistor_driver.readFrame(
            1000);

    REQUIRE_FALSE(acquisition.success());

    REQUIRE(
        acquisition.error ==
        Amg8833DriverError::
            ThermistorReadFailed);

    REQUIRE(
        acquisition.recovery_attempted);

    REQUIRE(
        acquisition.recovery_succeeded);

    REQUIRE(
        thermistor_driver.health()
            .total_failures ==
        1);
}

TEST_CASE(
    "Sensor overflow does not count as communication failure",
    "[amg8833_driver][recovery][overflow]")
{
    class OverflowBus final
        : public Amg8833Bus
    {
    public:
        OverflowBus()
            : registers_()
        {
            registers_[
                Amg8833Registers::STATUS] =
                Amg8833Registers::
                    STATUS_PIXEL_OVERFLOW_MASK;
        }

        bool writeRegister(
            std::uint8_t register_address,
            std::uint8_t value) override
        {
            registers_[register_address] =
                value;

            return true;
        }

        bool readRegisters(
            std::uint8_t start_register,
            std::uint8_t* destination,
            std::size_t length) override
        {
            for (std::size_t index = 0;
                 index < length;
                 ++index)
            {
                destination[index] =
                    registers_[
                        static_cast<std::size_t>(
                            start_register) +
                        index];
            }

            return true;
        }

    private:
        std::array<std::uint8_t, 256>
            registers_;
    };

    OverflowBus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE(acquisition.success());

    REQUIRE_FALSE(
        acquisition.frame.isValid());

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.total_failures ==
        0);

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(
        health.recovery_attempts ==
        0);

    REQUIRE(health.healthy());
}

TEST_CASE(
    "Manual initialization preserves lifetime health counters",
    "[amg8833_driver][recovery][initialization]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.failNextReads(1);

    REQUIRE_FALSE(
        driver.readFrame(1000)
            .success());

    REQUIRE(
        driver.health()
            .total_failures ==
        1);

    REQUIRE(driver.initialize());

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE(
        health.total_failures ==
        1);

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(health.initialized);
}

TEST_CASE(
    "Reset clears all driver health counters",
    "[amg8833_driver][recovery][reset]")
{
    RecoveryTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.failNextReads(2);

    REQUIRE_FALSE(
        driver.readFrame(1000)
            .success());

    REQUIRE_FALSE(
        driver.readFrame(1100)
            .success());

    REQUIRE(
        driver.health()
            .total_failures ==
        2);

    driver.reset();

    const Amg8833DriverHealth health =
        driver.health();

    REQUIRE_FALSE(health.initialized);

    REQUIRE(
        health.consecutive_failures ==
        0);

    REQUIRE(
        health.total_failures ==
        0);

    REQUIRE(
        health.recovery_attempts ==
        0);

    REQUIRE(
        health.successful_recoveries ==
        0);

    REQUIRE(
        health.failed_recoveries ==
        0);
}