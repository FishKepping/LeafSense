#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/drivers/amg8833_driver.h"
#include "leafsense/drivers/amg8833_snapshot.h"
#include "leafsense/drivers/amg8833_telemetry.h"

namespace {

using leafsense::Amg8833Registers;

using leafsense::drivers::Amg8833Bus;
using leafsense::drivers::Amg8833Driver;
using leafsense::drivers::Amg8833DriverConfig;
using leafsense::drivers::Amg8833DriverError;
using leafsense::drivers::Amg8833Snapshot;
using leafsense::drivers::Amg8833SnapshotReader;
using leafsense::drivers::Amg8833Telemetry;
using leafsense::drivers::Amg8833TelemetryProjector;

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

class TelemetryTestBus final
    : public Amg8833Bus
{
public:
    TelemetryTestBus()
        : registers_(),
          fail_all_reads_(false),
          fail_interrupt_map_read_(false)
    {
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
        float remaining_temperature,
        float thermistor_temperature = 25.0f)
    {
        registers_[
            Amg8833Registers::STATUS] =
            0x00;

        setThermistorTemperature(
            thermistor_temperature);

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

    void setStatus(
        std::uint8_t status)
    {
        registers_[
            Amg8833Registers::STATUS] =
            status;
    }

    void setInterruptPixel(
        std::size_t pixel_index)
    {
        if (pixel_index >= 64U)
        {
            return;
        }

        const std::size_t byte_index =
            pixel_index / 8U;

        const std::size_t bit_index =
            pixel_index % 8U;

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
        fail_all_reads_ =
            fail;
    }

    void setFailInterruptMapRead(
        bool fail)
    {
        fail_interrupt_map_read_ =
            fail;
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
            pixel_index * 2U;

        registers_[address] =
            static_cast<std::uint8_t>(
                encoded & 0x00FFU);

        registers_[address + 1U] =
            static_cast<std::uint8_t>(
                (encoded >> 8U) &
                0x000FU);
    }

    void setThermistorTemperature(
        float temperature)
    {
        const std::int16_t counts =
            static_cast<std::int16_t>(
                std::lround(
                    temperature / 0.0625f));

        const std::uint16_t encoded =
            static_cast<std::uint16_t>(
                counts) &
            0x0FFFU;

        registers_[
            Amg8833Registers::THERMISTOR_LOW] =
            static_cast<std::uint8_t>(
                encoded & 0x00FFU);

        registers_[
            Amg8833Registers::THERMISTOR_HIGH] =
            static_cast<std::uint8_t>(
                (encoded >> 8U) &
                0x000FU);
    }

    std::array<std::uint8_t, 256>
        registers_;

    bool fail_all_reads_;

    bool fail_interrupt_map_read_;
};

}  // namespace

TEST_CASE(
    "Telemetry projects valid snapshot measurements",
    "[amg8833_telemetry][measurements]")
{
    TelemetryTestBus bus;

    bus.prepareFrame(
        10.0f,
        20.0f,
        25.0f);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Snapshot snapshot =
        reader.capture(
            1234);

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            snapshot);

    REQUIRE(
        telemetry.frame_read_succeeded);

    REQUIRE(
        telemetry.frame_available);

    REQUIRE(
        telemetry.snapshot_complete);

    REQUIRE(
        telemetry.temperature_values_available);

    REQUIRE(
        telemetry.temperaturesAvailable());

    REQUIRE(
        temperaturesEqual(
            telemetry.minimum_temperature,
            10.0f));

    REQUIRE(
        temperaturesEqual(
            telemetry.maximum_temperature,
            20.0f));

    REQUIRE(
        temperaturesEqual(
            telemetry.average_temperature,
            19.84375f));

    REQUIRE(
        temperaturesEqual(
            telemetry.thermistor_temperature,
            25.0f));

    REQUIRE(
        telemetry.valid_pixel_count ==
        64U);

    REQUIRE(
        telemetry.frame_number ==
        1U);

    REQUIRE(
        telemetry.timestamp_ms ==
        1234U);

    REQUIRE(
        telemetry.driver_initialized);

    REQUIRE(
        telemetry.driver_healthy);

    REQUIRE(
        telemetry.fullyOperational());
}

TEST_CASE(
    "Telemetry reports unavailable values before initialization",
    "[amg8833_telemetry][availability]")
{
    TelemetryTestBus bus;

    Amg8833Driver driver(
        bus);

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000));

    REQUIRE_FALSE(
        telemetry.frame_read_succeeded);

    REQUIRE_FALSE(
        telemetry.frame_available);

    REQUIRE_FALSE(
        telemetry.snapshot_complete);

    REQUIRE_FALSE(
        telemetry.temperature_values_available);

    REQUIRE_FALSE(
        telemetry.temperaturesAvailable());

    REQUIRE_FALSE(
        telemetry.fullyOperational());

    REQUIRE(
        telemetry.frame_error ==
        Amg8833DriverError::
            NotInitialized);

    REQUIRE_FALSE(
        telemetry.driver_initialized);

    REQUIRE_FALSE(
        telemetry.driver_healthy);
}

TEST_CASE(
    "Telemetry reports sensor overflow without publishing temperatures",
    "[amg8833_telemetry][overflow]")
{
    TelemetryTestBus bus;

    bus.prepareFrame(
        20.0f,
        20.0f);

    bus.setStatus(
        Amg8833Registers::
            STATUS_PIXEL_OVERFLOW_MASK);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000));

    REQUIRE(
        telemetry.frame_read_succeeded);

    REQUIRE_FALSE(
        telemetry.frame_available);

    REQUIRE(
        telemetry.snapshot_complete);

    REQUIRE_FALSE(
        telemetry.temperature_values_available);

    REQUIRE_FALSE(
        telemetry.temperaturesAvailable());

    REQUIRE(
        telemetry.pixel_temperature_overflow);

    REQUIRE_FALSE(
        telemetry.thermistor_overflow);

    REQUIRE(
        telemetry.overflowDetected());

    REQUIRE_FALSE(
        telemetry.fullyOperational());

    REQUIRE(
        telemetry.total_failures ==
        0U);
}

TEST_CASE(
    "Telemetry projects interrupt status and interrupt pixel count",
    "[amg8833_telemetry][interrupt]")
{
    TelemetryTestBus bus;

    bus.prepareFrame(
        20.0f,
        20.0f);

    bus.setStatus(
        Amg8833Registers::
            STATUS_INTERRUPT_MASK);

    bus.setInterruptPixel(
        0);

    bus.setInterruptPixel(
        9);

    bus.setInterruptPixel(
        63);

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    Amg8833SnapshotReader reader(
        driver);

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000,
                true));

    REQUIRE(
        telemetry.interrupt_map_requested);

    REQUIRE(
        telemetry.interrupt_map_available);

    REQUIRE(
        telemetry.sensor_interrupt_active);

    REQUIRE(
        telemetry.any_interrupt_pixel_active);

    REQUIRE(
        telemetry.active_interrupt_pixel_count ==
        3U);

    REQUIRE(
        telemetry.interruptDetected());

    REQUIRE(
        telemetry.snapshot_complete);
}

TEST_CASE(
    "Telemetry distinguishes optional interrupt map failure",
    "[amg8833_telemetry][interrupt][failure]")
{
    TelemetryTestBus bus;

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

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000,
                true));

    REQUIRE(
        telemetry.frame_read_succeeded);

    REQUIRE(
        telemetry.frame_available);

    REQUIRE(
        telemetry.temperaturesAvailable());

    REQUIRE(
        telemetry.interrupt_map_requested);

    REQUIRE_FALSE(
        telemetry.interrupt_map_available);

    REQUIRE_FALSE(
        telemetry.snapshot_complete);

    REQUIRE(
        telemetry.interrupt_map_error ==
        Amg8833DriverError::
            InterruptTableReadFailed);

    REQUIRE(
        telemetry.consecutive_failures ==
        1U);

    REQUIRE(
        telemetry.total_failures ==
        1U);

    REQUIRE_FALSE(
        telemetry.driver_healthy);

    REQUIRE_FALSE(
        telemetry.fullyOperational());
}

TEST_CASE(
    "Telemetry projects failed frame acquisition",
    "[amg8833_telemetry][failure]")
{
    TelemetryTestBus bus;

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

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000));

    REQUIRE_FALSE(
        telemetry.frame_read_succeeded);

    REQUIRE_FALSE(
        telemetry.frame_available);

    REQUIRE_FALSE(
        telemetry.snapshot_complete);

    REQUIRE_FALSE(
        telemetry.temperaturesAvailable());

    REQUIRE(
        telemetry.frame_error ==
        Amg8833DriverError::
            StatusReadFailed);

    REQUIRE(
        telemetry.consecutive_failures ==
        1U);

    REQUIRE(
        telemetry.total_failures ==
        1U);

    REQUIRE_FALSE(
        telemetry.driver_healthy);
}

TEST_CASE(
    "Telemetry exposes automatic recovery state",
    "[amg8833_telemetry][recovery]")
{
    TelemetryTestBus bus;

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

    const Amg8833Telemetry telemetry =
        Amg8833TelemetryProjector::project(
            reader.capture(
                1000));

    REQUIRE(
        telemetry.recovery_attempted);

    REQUIRE(
        telemetry.recovery_succeeded);

    REQUIRE(
        telemetry.recovery_attempts ==
        1U);

    REQUIRE(
        telemetry.successful_recoveries ==
        1U);

    REQUIRE(
        telemetry.failed_recoveries ==
        0U);

    REQUIRE(
        telemetry.driver_initialized);
}

TEST_CASE(
    "Telemetry helper methods combine diagnostic flags",
    "[amg8833_telemetry][helpers]")
{
    Amg8833Telemetry telemetry;

    REQUIRE_FALSE(
        telemetry.overflowDetected());

    REQUIRE_FALSE(
        telemetry.interruptDetected());

    telemetry.thermistor_overflow =
        true;

    telemetry.any_interrupt_pixel_active =
        true;

    REQUIRE(
        telemetry.overflowDetected());

    REQUIRE(
        telemetry.interruptDetected());
}