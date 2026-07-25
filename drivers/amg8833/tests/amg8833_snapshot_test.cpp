#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/drivers/amg8833_driver.h"
#include "leafsense/drivers/amg8833_snapshot.h"

namespace {

using leafsense::Amg8833Registers;

using leafsense::drivers::Amg8833Bus;
using leafsense::drivers::Amg8833Driver;
using leafsense::drivers::Amg8833DriverConfig;
using leafsense::drivers::Amg8833DriverError;
using leafsense::drivers::Amg8833Snapshot;
using leafsense::drivers::Amg8833SnapshotReader;

constexpr float kTemperatureTolerance =
    0.0001f;

bool temperaturesEqual(
    float actual,
    float expected)
{
    return std::fabs(
        actual - expected) <=
        kTemperatureTolerance;
}

class SnapshotTestBus final
    : public Amg8833Bus
{
public:
    SnapshotTestBus()
        : registers_(),
          read_count_(0),
          write_count_(0),
          fail_all_reads_(false),
          fail_interrupt_map_read_(false)
    {
    }

    bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) override
    {
        ++write_count_;

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

        if (fail_all_reads_)
        {
            return false;
        }

        if (fail_interrupt_map_read_ &&
            start_register ==
                Amg8833Registers::
                    INTERRUPT_TABLE_START)
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

    void prepareFrame(
        float first_temperature,
        float remaining_temperature)
    {
        registers_[
            Amg8833Registers::STATUS] =
            0x00;

        /*
         * Thermistor raw value:
         *
         *     0x190 × 0.0625 °C = 25.0 °C
         */
        registers_[
            Amg8833Registers::THERMISTOR_LOW] =
            0x90;

        registers_[
            Amg8833Registers::THERMISTOR_HIGH] =
            0x01;

        setPixelTemperature(
            0,
            first_temperature);

        for (std::size_t pixel = 1;
             pixel < 64;
             ++pixel)
        {
            setPixelTemperature(
                pixel,
                remaining_temperature);
        }
    }

    void prepareOverflowFrame()
    {
        prepareFrame(
            20.0f,
            20.0f);

        registers_[
            Amg8833Registers::STATUS] =
            Amg8833Registers::
                STATUS_PIXEL_OVERFLOW_MASK;
    }

    void setInterruptPixel(
        std::size_t pixel_index)
    {
        if (pixel_index >= 64)
        {
            return;
        }

        const std::size_t byte_index =
            pixel_index / 8;

        const std::size_t bit_index =
            pixel_index % 8;

        const std::size_t address =
            static_cast<std::size_t>(
                Amg8833Registers::
                    INTERRUPT_TABLE_START) +
            byte_index;

        registers_[address] |=
            static_cast<std::uint8_t>(
                1U << bit_index);
    }

    void setFailAllReads(
        bool fail)
    {
        fail_all_reads_ = fail;
    }

    void setFailInterruptMapRead(
        bool fail)
    {
        fail_interrupt_map_read_ =
            fail;
    }

    std::size_t readCount() const
    {
        return read_count_;
    }

private:
    void setPixelTemperature(
        std::size_t pixel_index,
        float temperature)
    {
        const std::int16_t counts =
            static_cast<std::int16_t>(
                std::lround(
                    temperature / 0.25f));

        const std::uint16_t encoded =
            static_cast<std::uint16_t>(
                counts) &
            0x0FFFU;

        const std::size_t address =
            static_cast<std::size_t>(
                Amg8833Registers::
                    PIXEL_TEMPERATURE_START) +
            pixel_index * 2;

        registers_[address] =
            static_cast<std::uint8_t>(
                encoded & 0x00FFU);

        registers_[address + 1] =
            static_cast<std::uint8_t>(
                (encoded >> 8U) &
                0x000FU);
    }

    std::array<std::uint8_t, 256>
        registers_;

    std::size_t read_count_;

    std::size_t write_count_;

    bool fail_all_reads_;

    bool fail_interrupt_map_read_;
};

}  // namespace

TEST_CASE(
    "Snapshot reports an uninitialized driver",
    "[amg8833_snapshot][initialization]")
{
    SnapshotTestBus bus;

    Amg8833Driver driver(
        bus);

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000);

    REQUIRE_FALSE(
        snapshot.frameReadSucceeded());

    REQUIRE_FALSE(
        snapshot.frameAvailable());

    REQUIRE_FALSE(
        snapshot.complete());

    REQUIRE(
        snapshot.frame_error ==
        Amg8833DriverError::
            NotInitialized);

    REQUIRE_FALSE(
        snapshot.summary.available);

    REQUIRE_FALSE(
        snapshot.health.initialized);
}

TEST_CASE(
    "Snapshot calculates whole-frame statistics",
    "[amg8833_snapshot][statistics]")
{
    SnapshotTestBus bus;

    bus.prepareFrame(
        10.0f,
        20.0f);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1234);

    REQUIRE(
        snapshot.frameReadSucceeded());

    REQUIRE(
        snapshot.frameAvailable());

    REQUIRE(
        snapshot.complete());

    REQUIRE(
        snapshot.frame.frameNumber() ==
        1);

    REQUIRE(
        snapshot.frame.timestampMs() ==
        1234);

    REQUIRE(
        snapshot.summary.available);

    REQUIRE(
        snapshot.summary.valid_pixel_count ==
        64);

    REQUIRE(
        snapshot.summary.minimum_temperature ==
        10.0f);

    REQUIRE(
        snapshot.summary.maximum_temperature ==
        20.0f);

    REQUIRE(
        temperaturesEqual(
            snapshot.summary.average_temperature,
            19.84375f));

    REQUIRE(
        temperaturesEqual(
            snapshot.summary.thermistor_temperature,
            25.0f));

    REQUIRE(
        snapshot.health.initialized);

    REQUIRE(
        snapshot.health.healthy());
}

TEST_CASE(
    "Snapshot omits interrupt map by default",
    "[amg8833_snapshot][interrupt]")
{
    SnapshotTestBus bus;

    bus.prepareFrame(
        20.0f,
        20.0f);

    bus.setInterruptPixel(
        0);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const std::size_t reads_before =
        bus.readCount();

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000);

    REQUIRE_FALSE(
        snapshot.interrupt_map_requested);

    REQUIRE_FALSE(
        snapshot.interrupt_map_available);

    REQUIRE(
        snapshot.interrupt_map.empty());

    REQUIRE(
        bus.readCount() ==
        reads_before + 3);

    REQUIRE(snapshot.complete());
}

TEST_CASE(
    "Snapshot includes requested interrupt map",
    "[amg8833_snapshot][interrupt]")
{
    SnapshotTestBus bus;

    bus.prepareFrame(
        20.0f,
        20.0f);

    bus.setInterruptPixel(
        0);

    bus.setInterruptPixel(
        27);

    bus.setInterruptPixel(
        63);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000,
            true);

    REQUIRE(
        snapshot.frameReadSucceeded());

    REQUIRE(
        snapshot.interrupt_map_requested);

    REQUIRE(
        snapshot.interrupt_map_available);

    REQUIRE(
        snapshot.interrupt_map_error ==
        Amg8833DriverError::None);

    REQUIRE(
        snapshot.interrupt_map.activeCount() ==
        3);

    REQUIRE(
        snapshot.interrupt_map.active(0));

    REQUIRE(
        snapshot.interrupt_map.active(27));

    REQUIRE(
        snapshot.interrupt_map.active(63));

    REQUIRE(snapshot.complete());
}

TEST_CASE(
    "Frame failure prevents interrupt map read",
    "[amg8833_snapshot][failure]")
{
    SnapshotTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.setFailAllReads(
        true);

    Amg8833SnapshotReader reader(
        driver);

    const std::size_t reads_before =
        bus.readCount();

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000,
            true);

    REQUIRE_FALSE(
        snapshot.frameReadSucceeded());

    REQUIRE(
        snapshot.frame_error ==
        Amg8833DriverError::
            StatusReadFailed);

    REQUIRE(
        snapshot.interrupt_map_requested);

    REQUIRE_FALSE(
        snapshot.interrupt_map_available);

    REQUIRE(
        snapshot.interrupt_map_error ==
        Amg8833DriverError::None);

    REQUIRE(
        bus.readCount() ==
        reads_before + 1);

    REQUIRE_FALSE(snapshot.complete());

    REQUIRE(
        snapshot.health.total_failures ==
        1);
}

TEST_CASE(
    "Interrupt map failure preserves successful frame",
    "[amg8833_snapshot][interrupt][failure]")
{
    SnapshotTestBus bus;

    bus.prepareFrame(
        15.0f,
        25.0f);

    Amg8833DriverConfig config;

    config.recovery.enabled =
        false;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.setFailInterruptMapRead(
        true);

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000,
            true);

    REQUIRE(
        snapshot.frameReadSucceeded());

    REQUIRE(
        snapshot.frameAvailable());

    REQUIRE(
        snapshot.summary.available);

    REQUIRE_FALSE(
        snapshot.interrupt_map_available);

    REQUIRE(
        snapshot.interrupt_map_error ==
        Amg8833DriverError::
            InterruptTableReadFailed);

    REQUIRE_FALSE(snapshot.complete());

    REQUIRE(
        snapshot.health.total_failures ==
        1);

    REQUIRE(
        snapshot.health.consecutive_failures ==
        1);
}

TEST_CASE(
    "Overflow frame has no available summary",
    "[amg8833_snapshot][overflow]")
{
    SnapshotTestBus bus;

    bus.prepareOverflowFrame();

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000);

    /*
     * All bus operations succeeded.
     */
    REQUIRE(
        snapshot.frameReadSucceeded());

    REQUIRE(snapshot.complete());

    /*
     * Sensor overflow makes the thermal data unusable.
     */
    REQUIRE_FALSE(
        snapshot.frameAvailable());

    REQUIRE_FALSE(
        snapshot.frame.isValid());

    REQUIRE_FALSE(
        snapshot.summary.available);

    REQUIRE(
        snapshot.summary.valid_pixel_count ==
        0);

    REQUIRE(
        snapshot.health.total_failures ==
        0);
}

TEST_CASE(
    "Snapshot exposes successful frame recovery",
    "[amg8833_snapshot][recovery]")
{
    SnapshotTestBus bus;

    Amg8833DriverConfig config;

    config.recovery.enabled =
        true;

    config.recovery.failure_threshold =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    bus.setFailAllReads(
        true);

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000);

    REQUIRE_FALSE(
        snapshot.frameReadSucceeded());

    REQUIRE(
        snapshot.frame_recovery_attempted);

    REQUIRE(
        snapshot.frame_recovery_succeeded);

    REQUIRE(
        snapshot.recoveryAttempted());

    REQUIRE(
        snapshot.recoverySucceeded());

    REQUIRE(
        snapshot.health.successful_recoveries ==
        1);
}

TEST_CASE(
    "Snapshot reports no recovery success when none was attempted",
    "[amg8833_snapshot][recovery]")
{
    SnapshotTestBus bus;

    bus.prepareFrame(
        20.0f,
        20.0f);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1000);

    REQUIRE_FALSE(
        snapshot.recoveryAttempted());

    REQUIRE_FALSE(
        snapshot.recoverySucceeded());
}