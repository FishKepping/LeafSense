#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "leafsense/amg8833_registers.h"
#include "leafsense/drivers/amg8833_bus.h"
#include "leafsense/drivers/amg8833_driver.h"
#include "leafsense/thermal_frame.h"
#include "leafsense/thermal_processor.h"

namespace {

using Catch::Matchers::WithinAbs;

using leafsense::Amg8833FrameRate;
using leafsense::Amg8833InterruptMode;
using leafsense::Amg8833Registers;
using leafsense::ProcessingConfig;
using leafsense::SpatialFilter;
using leafsense::ThermalFrame;

using leafsense::drivers::Amg8833Acquisition;
using leafsense::drivers::Amg8833Bus;
using leafsense::drivers::Amg8833Driver;
using leafsense::drivers::Amg8833DriverConfig;
using leafsense::drivers::Amg8833DriverError;

constexpr float TEST_TOLERANCE =
    0.0001f;

struct WriteOperation
{
    std::uint8_t register_address;
    std::uint8_t value;
};

struct ReadOperation
{
    std::uint8_t start_register;
    std::size_t length;
};

class FakeAmg8833Bus final
    : public Amg8833Bus
{
public:
    static constexpr std::size_t NO_FAILURE =
        std::numeric_limits<std::size_t>::max();

    FakeAmg8833Bus()
        : registers_(),
          write_operations_(),
          read_operations_(),
          fail_write_call_(NO_FAILURE),
          fail_read_call_(NO_FAILURE)
    {
    }

    bool writeRegister(
        std::uint8_t register_address,
        std::uint8_t value) override
    {
        const std::size_t call_index =
            write_operations_.size();

        write_operations_.push_back(
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

        if (register_address ==
            Amg8833Registers::STATUS_CLEAR)
        {
            registers_[
                Amg8833Registers::STATUS] &=
                static_cast<std::uint8_t>(
                    ~value);
        }

        if (register_address ==
                Amg8833Registers::RESET &&
            (value ==
                 static_cast<std::uint8_t>(
                     leafsense::
                         Amg8833ResetCommand::
                             FlagReset) ||
             value ==
                 static_cast<std::uint8_t>(
                     leafsense::
                         Amg8833ResetCommand::
                             InitialReset)))
        {
            registers_[
                Amg8833Registers::STATUS] =
                0;
        }

        return true;
    }

    bool readRegisters(
        std::uint8_t start_register,
        std::uint8_t* destination,
        std::size_t length) override
    {
        const std::size_t call_index =
            read_operations_.size();

        read_operations_.push_back(
            ReadOperation{
                start_register,
                length});

        if (call_index ==
            fail_read_call_)
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

    void setRegister(
        std::uint8_t register_address,
        std::uint8_t value)
    {
        registers_[register_address] =
            value;
    }

    std::uint8_t registerValue(
        std::uint8_t register_address) const
    {
        return registers_[register_address];
    }

    void failWriteAt(
        std::size_t call_index)
    {
        fail_write_call_ =
            call_index;
    }

    void failReadAt(
        std::size_t call_index)
    {
        fail_read_call_ =
            call_index;
    }

    void clearFailures()
    {
        fail_write_call_ =
            NO_FAILURE;

        fail_read_call_ =
            NO_FAILURE;
    }

    const std::vector<WriteOperation>&
    writeOperations() const
    {
        return write_operations_;
    }

    const std::vector<ReadOperation>&
    readOperations() const
    {
        return read_operations_;
    }

private:
    std::array<std::uint8_t, 256>
        registers_;

    std::vector<WriteOperation>
        write_operations_;

    std::vector<ReadOperation>
        read_operations_;

    std::size_t fail_write_call_;

    std::size_t fail_read_call_;
};

std::uint16_t encodePixelTemperature(
    float temperature)
{
    const auto counts =
        static_cast<std::int16_t>(
            temperature / 0.25f);

    return static_cast<std::uint16_t>(
        counts) &
        0x0FFFU;
}

std::uint16_t encodeThermistorTemperature(
    float temperature)
{
    const bool negative =
        temperature < 0.0f;

    const float magnitude_temperature =
        negative
            ? -temperature
            : temperature;

    const auto magnitude =
        static_cast<std::uint16_t>(
            magnitude_temperature /
            0.0625f);

    return static_cast<std::uint16_t>(
        (negative ? 0x0800U : 0x0000U) |
        (magnitude & 0x07FFU));
}

void setRawPixel(
    FakeAmg8833Bus& bus,
    std::size_t pixel_index,
    std::uint16_t raw_value)
{
    const std::size_t register_index =
        static_cast<std::size_t>(
            Amg8833Registers::
                PIXEL_TEMPERATURE_START) +
        pixel_index * 2;

    bus.setRegister(
        static_cast<std::uint8_t>(
            register_index),
        static_cast<std::uint8_t>(
            raw_value & 0x00FFU));

    bus.setRegister(
        static_cast<std::uint8_t>(
            register_index + 1),
        static_cast<std::uint8_t>(
            (raw_value >> 8U) &
            0x000FU));
}

void setUniformPixels(
    FakeAmg8833Bus& bus,
    float temperature)
{
    const std::uint16_t raw_value =
        encodePixelTemperature(
            temperature);

    for (std::size_t pixel_index = 0;
         pixel_index <
             ThermalFrame::PIXEL_COUNT;
         ++pixel_index)
    {
        setRawPixel(
            bus,
            pixel_index,
            raw_value);
    }
}

void setThermistor(
    FakeAmg8833Bus& bus,
    float temperature)
{
    const std::uint16_t raw_value =
        encodeThermistorTemperature(
            temperature);

    bus.setRegister(
        Amg8833Registers::THERMISTOR_LOW,
        static_cast<std::uint8_t>(
            raw_value & 0x00FFU));

    bus.setRegister(
        Amg8833Registers::THERMISTOR_HIGH,
        static_cast<std::uint8_t>(
            (raw_value >> 8U) &
            0x000FU));
}

void requireWrite(
    const FakeAmg8833Bus& bus,
    std::size_t operation_index,
    std::uint8_t register_address,
    std::uint8_t value)
{
    REQUIRE(
        operation_index <
        bus.writeOperations().size());

    REQUIRE(
        bus.writeOperations()
            [operation_index]
                .register_address ==
        register_address);

    REQUIRE(
        bus.writeOperations()
            [operation_index]
                .value ==
        value);
}

void requireRead(
    const FakeAmg8833Bus& bus,
    std::size_t operation_index,
    std::uint8_t start_register,
    std::size_t length)
{
    REQUIRE(
        operation_index <
        bus.readOperations().size());

    REQUIRE(
        bus.readOperations()
            [operation_index]
                .start_register ==
        start_register);

    REQUIRE(
        bus.readOperations()
            [operation_index]
                .length ==
        length);
}

}  // namespace

TEST_CASE(
    "Amg8833Driver has safe default configuration",
    "[amg8833_driver][configuration]")
{
    FakeAmg8833Bus bus;

    const Amg8833Driver driver(
        bus);

    REQUIRE(
        driver.config().frame_rate ==
        Amg8833FrameRate::
            FramesPerSecond10);

    REQUIRE_FALSE(
        driver.config()
            .moving_average_enabled);

    REQUIRE_FALSE(
        driver.config()
            .interrupt.enabled);

    REQUIRE(
        driver.config()
            .interrupt.mode ==
        Amg8833InterruptMode::Difference);

    REQUIRE(
        driver.config()
            .processing.spatial_filter ==
        SpatialFilter::None);

    REQUIRE_FALSE(
        driver.config()
            .processing
            .exponential_enabled);

    REQUIRE_FALSE(
        driver.initialized());

    REQUIRE(
        driver.frameCount() ==
        0);

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);
}

TEST_CASE(
    "Amg8833Driver performs the default initialization sequence",
    "[amg8833_driver][initialization]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());
    REQUIRE(driver.initialized());

    REQUIRE(
        bus.writeOperations().size() ==
        10);

    requireWrite(
        bus,
        0,
        Amg8833Registers::POWER_CONTROL,
        0x00);

    requireWrite(
        bus,
        1,
        Amg8833Registers::RESET,
        0x3F);

    requireWrite(
        bus,
        2,
        Amg8833Registers::FRAME_RATE,
        0x00);

    requireWrite(
        bus,
        3,
        Amg8833Registers::
            RESERVED_AVERAGE_CONTROL,
        0x50);

    requireWrite(
        bus,
        4,
        Amg8833Registers::
            RESERVED_AVERAGE_CONTROL,
        0x45);

    requireWrite(
        bus,
        5,
        Amg8833Registers::
            RESERVED_AVERAGE_CONTROL,
        0x57);

    requireWrite(
        bus,
        6,
        Amg8833Registers::AVERAGE,
        0x00);

    requireWrite(
        bus,
        7,
        Amg8833Registers::
            RESERVED_AVERAGE_CONTROL,
        0x00);

    requireWrite(
        bus,
        8,
        Amg8833Registers::
            INTERRUPT_CONTROL,
        0x00);

    requireWrite(
        bus,
        9,
        Amg8833Registers::STATUS_CLEAR,
        0x0E);

    REQUIRE(
        bus.readOperations().empty());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);
}

TEST_CASE(
    "Amg8833Driver initializes custom hardware configuration",
    "[amg8833_driver][initialization][configuration]")
{
    FakeAmg8833Bus bus;

    Amg8833DriverConfig config;

    config.frame_rate =
        Amg8833FrameRate::
            FramesPerSecond1;

    config.moving_average_enabled =
        true;

    config.interrupt.enabled =
        true;

    config.interrupt.mode =
        Amg8833InterruptMode::Absolute;

    config.processing.spatial_filter =
        SpatialFilter::Median;

    config.processing.spatial_radius =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    requireWrite(
        bus,
        2,
        Amg8833Registers::FRAME_RATE,
        0x01);

    requireWrite(
        bus,
        6,
        Amg8833Registers::AVERAGE,
        0x20);

    requireWrite(
        bus,
        8,
        Amg8833Registers::
            INTERRUPT_CONTROL,
        0x03);

    REQUIRE(
        driver.config()
            .processing.spatial_filter ==
        SpatialFilter::Median);
}

TEST_CASE(
    "Amg8833Driver reports each initialization write failure",
    "[amg8833_driver][initialization][error]")
{
    struct FailureCase
    {
        std::size_t write_call;
        Amg8833DriverError error;
    };

    const std::array<FailureCase, 10>
        failure_cases{{
            {
                0,
                Amg8833DriverError::
                    PowerControlWriteFailed},
            {
                1,
                Amg8833DriverError::
                    InitialResetWriteFailed},
            {
                2,
                Amg8833DriverError::
                    FrameRateWriteFailed},
            {
                3,
                Amg8833DriverError::
                    MovingAverageWriteFailed},
            {
                4,
                Amg8833DriverError::
                    MovingAverageWriteFailed},
            {
                5,
                Amg8833DriverError::
                    MovingAverageWriteFailed},
            {
                6,
                Amg8833DriverError::
                    MovingAverageWriteFailed},
            {
                7,
                Amg8833DriverError::
                    MovingAverageWriteFailed},
            {
                8,
                Amg8833DriverError::
                    InterruptControlWriteFailed},
            {
                9,
                Amg8833DriverError::
                    StatusClearWriteFailed},
        }};

    for (const FailureCase& failure_case :
         failure_cases)
    {
        DYNAMIC_SECTION(
            "write call "
            << failure_case.write_call)
        {
            FakeAmg8833Bus bus;

            bus.failWriteAt(
                failure_case.write_call);

            Amg8833Driver driver(
                bus);

            REQUIRE_FALSE(
                driver.initialize());

            REQUIRE_FALSE(
                driver.initialized());

            REQUIRE(
                driver.lastError() ==
                failure_case.error);

            REQUIRE(
                bus.writeOperations().size() ==
                failure_case.write_call + 1);
        }
    }
}

TEST_CASE(
    "Amg8833Driver initialization can be retried",
    "[amg8833_driver][initialization][recovery]")
{
    FakeAmg8833Bus bus;

    bus.failWriteAt(0);

    Amg8833Driver driver(
        bus);

    REQUIRE_FALSE(
        driver.initialize());

    bus.clearFailures();

    REQUIRE(driver.initialize());
    REQUIRE(driver.initialized());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);

    REQUIRE(
        driver.frameCount() ==
        0);
}

TEST_CASE(
    "Amg8833Driver rejects acquisition before initialization",
    "[amg8833_driver][acquisition][error]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE_FALSE(
        acquisition.success());

    REQUIRE(
        acquisition.error ==
        Amg8833DriverError::
            NotInitialized);

    REQUIRE_FALSE(
        acquisition.frame.isValid());

    REQUIRE(
        bus.readOperations().empty());

    REQUIRE(
        driver.frameCount() ==
        0);
}

TEST_CASE(
    "Amg8833Driver acquires and processes a complete frame",
    "[amg8833_driver][acquisition]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        0x00);

    setThermistor(
        bus,
        26.5f);

    setUniformPixels(
        bus,
        25.0f);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            123456);

    REQUIRE(acquisition.success());
    REQUIRE(acquisition.frame.isValid());

    REQUIRE(
        acquisition.frame.frameNumber() ==
        1);

    REQUIRE(
        acquisition.frame.timestampMs() ==
        123456);

    REQUIRE_THAT(
        acquisition.frame
            .thermistorTemperature(),
        WithinAbs(
            26.5f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        acquisition.frame.pixel(0, 0),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));

    REQUIRE_THAT(
        acquisition.frame.pixel(7, 7),
        WithinAbs(
            25.0f,
            TEST_TOLERANCE));

    REQUIRE(
        acquisition.status.clear());

    REQUIRE(
        driver.lastStatus().clear());

    REQUIRE(
        driver.frameCount() ==
        1);

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);
}

TEST_CASE(
    "Amg8833Driver reads the required register ranges",
    "[amg8833_driver][acquisition][bus]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    setUniformPixels(
        bus,
        20.0f);

    setThermistor(
        bus,
        20.0f);

    REQUIRE(
        driver.readFrame(1000)
            .success());

    REQUIRE(
        bus.readOperations().size() ==
        3);

    requireRead(
        bus,
        0,
        Amg8833Registers::STATUS,
        1);

    requireRead(
        bus,
        1,
        Amg8833Registers::THERMISTOR_LOW,
        2);

    requireRead(
        bus,
        2,
        Amg8833Registers::
            PIXEL_TEMPERATURE_START,
        128);
}

TEST_CASE(
    "Amg8833Driver increments frame numbers after complete acquisitions",
    "[amg8833_driver][acquisition][metadata]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        10.0f);

    const Amg8833Acquisition first =
        driver.readFrame(
            1000);

    setUniformPixels(
        bus,
        20.0f);

    const Amg8833Acquisition second =
        driver.readFrame(
            1100);

    REQUIRE(first.success());
    REQUIRE(second.success());

    REQUIRE(
        first.frame.frameNumber() ==
        1);

    REQUIRE(
        second.frame.frameNumber() ==
        2);

    REQUIRE(
        first.frame.timestampMs() ==
        1000);

    REQUIRE(
        second.frame.timestampMs() ==
        1100);

    REQUIRE(
        driver.frameCount() ==
        2);
}

TEST_CASE(
    "AMG8833 interrupt status does not invalidate frame data",
    "[amg8833_driver][status][interrupt]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        Amg8833Registers::
            STATUS_INTERRUPT_MASK);

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        25.0f);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE(acquisition.success());
    REQUIRE(acquisition.status.interrupt);

    REQUIRE_FALSE(
        acquisition.status
            .pixelTemperatureOverflow);

    REQUIRE_FALSE(
        acquisition.status
            .thermistorOverflow);

    REQUIRE(acquisition.frame.isValid());
}

TEST_CASE(
    "AMG8833 pixel overflow produces an invalid acquired frame",
    "[amg8833_driver][status][overflow]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        Amg8833Registers::
            STATUS_PIXEL_OVERFLOW_MASK);

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        25.0f);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE(acquisition.success());

    REQUIRE(
        acquisition.status
            .pixelTemperatureOverflow);

    REQUIRE_FALSE(
        acquisition.frame.isValid());

    REQUIRE(
        acquisition.frame.frameNumber() ==
        1);

    REQUIRE(
        driver.frameCount() ==
        1);
}

TEST_CASE(
    "AMG8833 thermistor overflow produces an invalid acquired frame",
    "[amg8833_driver][status][overflow]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        Amg8833Registers::
            STATUS_THERMISTOR_OVERFLOW_MASK);

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        25.0f);

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE(acquisition.success());

    REQUIRE(
        acquisition.status
            .thermistorOverflow);

    REQUIRE_FALSE(
        acquisition.frame.isValid());
}

TEST_CASE(
    "Invalid overflow frames do not advance exponential history",
    "[amg8833_driver][status][exponential]")
{
    FakeAmg8833Bus bus;

    Amg8833DriverConfig config;

    config.processing
        .exponential_enabled =
        true;

    config.processing
        .exponential_alpha =
        0.5f;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    setThermistor(
        bus,
        20.0f);

    bus.setRegister(
        Amg8833Registers::STATUS,
        0x00);

    setUniformPixels(
        bus,
        10.0f);

    const Amg8833Acquisition first =
        driver.readFrame(
            1000);

    REQUIRE(first.success());

    REQUIRE_THAT(
        first.frame.pixel(0, 0),
        WithinAbs(
            10.0f,
            TEST_TOLERANCE));

    bus.setRegister(
        Amg8833Registers::STATUS,
        Amg8833Registers::
            STATUS_PIXEL_OVERFLOW_MASK);

    setUniformPixels(
        bus,
        100.0f);

    const Amg8833Acquisition invalid =
        driver.readFrame(
            1100);

    REQUIRE(invalid.success());

    REQUIRE_FALSE(
        invalid.frame.isValid());

    bus.setRegister(
        Amg8833Registers::STATUS,
        0x00);

    setUniformPixels(
        bus,
        30.0f);

    const Amg8833Acquisition third =
        driver.readFrame(
            1200);

    REQUIRE(third.success());

    REQUIRE_THAT(
        third.frame.pixel(0, 0),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));

    REQUIRE(
        third.frame.frameNumber() ==
        3);
}

TEST_CASE(
    "Amg8833Driver applies configured spatial processing",
    "[amg8833_driver][processing][median]")
{
    FakeAmg8833Bus bus;

    Amg8833DriverConfig config;

    config.processing.spatial_filter =
        SpatialFilter::Median;

    config.processing.spatial_radius =
        1;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        20.0f);

    setRawPixel(
        bus,
        27,
        encodePixelTemperature(
            100.0f));

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1000);

    REQUIRE(acquisition.success());
    REQUIRE(acquisition.frame.isValid());

    REQUIRE_THAT(
        acquisition.frame.pixel(3, 3),
        WithinAbs(
            20.0f,
            TEST_TOLERANCE));
}

TEST_CASE(
    "Amg8833Driver reports acquisition read failures",
    "[amg8833_driver][acquisition][error]")
{
    struct FailureCase
    {
        std::size_t read_call;
        Amg8833DriverError error;
    };

    const std::array<FailureCase, 3>
        failure_cases{{
            {
                0,
                Amg8833DriverError::
                    StatusReadFailed},
            {
                1,
                Amg8833DriverError::
                    ThermistorReadFailed},
            {
                2,
                Amg8833DriverError::
                    PixelReadFailed},
        }};

    for (const FailureCase& failure_case :
         failure_cases)
    {
        DYNAMIC_SECTION(
            "read call "
            << failure_case.read_call)
        {
            FakeAmg8833Bus bus;

            Amg8833Driver driver(
                bus);

            REQUIRE(driver.initialize());

            setThermistor(
                bus,
                20.0f);

            setUniformPixels(
                bus,
                20.0f);

            bus.failReadAt(
                failure_case.read_call);

            const Amg8833Acquisition
                acquisition =
                    driver.readFrame(
                        1000);

            REQUIRE_FALSE(
                acquisition.success());

            REQUIRE(
                acquisition.error ==
                failure_case.error);

            REQUIRE_FALSE(
                acquisition.frame.isValid());

            REQUIRE(
                driver.lastError() ==
                failure_case.error);

            REQUIRE(
                driver.frameCount() ==
                0);

            REQUIRE(driver.initialized());
        }
    }
}

TEST_CASE(
    "Failed acquisition can be followed by a successful frame",
    "[amg8833_driver][acquisition][recovery]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        25.0f);

    bus.failReadAt(0);

    REQUIRE_FALSE(
        driver.readFrame(1000)
            .success());

    bus.clearFailures();

    const Amg8833Acquisition acquisition =
        driver.readFrame(
            1100);

    REQUIRE(acquisition.success());

    REQUIRE(
        acquisition.frame.frameNumber() ==
        1);

    REQUIRE(
        driver.frameCount() ==
        1);

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);
}

TEST_CASE(
    "Amg8833Driver clears active sensor status flags",
    "[amg8833_driver][status][clear]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        0x0E);

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        20.0f);

    REQUIRE(
        driver.readFrame(1000)
            .success());

    const std::size_t writes_before =
        bus.writeOperations().size();

    REQUIRE(driver.clearStatus());

    REQUIRE(
        bus.writeOperations().size() ==
        writes_before + 1);

    requireWrite(
        bus,
        writes_before,
        Amg8833Registers::STATUS_CLEAR,
        0x0E);

    REQUIRE(
        bus.registerValue(
            Amg8833Registers::STATUS) ==
        0x00);

    REQUIRE(
        driver.lastStatus().clear());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);
}

TEST_CASE(
    "Clearing an empty status performs no bus write",
    "[amg8833_driver][status][clear]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    const std::size_t writes_before =
        bus.writeOperations().size();

    REQUIRE(driver.clearStatus());

    REQUIRE(
        bus.writeOperations().size() ==
        writes_before);
}

TEST_CASE(
    "Amg8833Driver rejects status clearing before initialization",
    "[amg8833_driver][status][clear][error]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE_FALSE(
        driver.clearStatus());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::
            NotInitialized);

    REQUIRE(
        bus.writeOperations().empty());
}

TEST_CASE(
    "Amg8833Driver reports status-clear write failure",
    "[amg8833_driver][status][clear][error]")
{
    FakeAmg8833Bus bus;

    Amg8833Driver driver(
        bus);

    REQUIRE(driver.initialize());

    bus.setRegister(
        Amg8833Registers::STATUS,
        Amg8833Registers::
            STATUS_INTERRUPT_MASK);

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        20.0f);

    REQUIRE(
        driver.readFrame(1000)
            .success());

    bus.failWriteAt(
        bus.writeOperations().size());

    REQUIRE_FALSE(
        driver.clearStatus());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::
            StatusClearWriteFailed);

    REQUIRE(
        driver.lastStatus().interrupt);
}

TEST_CASE(
    "Amg8833Driver reset clears local state and processing history",
    "[amg8833_driver][reset]")
{
    FakeAmg8833Bus bus;

    Amg8833DriverConfig config;

    config.processing
        .exponential_enabled =
        true;

    config.processing
        .exponential_alpha =
        0.5f;

    Amg8833Driver driver(
        bus,
        config);

    REQUIRE(driver.initialize());

    setThermistor(
        bus,
        20.0f);

    setUniformPixels(
        bus,
        0.0f);

    REQUIRE(
        driver.readFrame(1000)
            .success());

    setUniformPixels(
        bus,
        100.0f);

    const Amg8833Acquisition second =
        driver.readFrame(
            1100);

    REQUIRE_THAT(
        second.frame.pixel(0, 0),
        WithinAbs(
            50.0f,
            TEST_TOLERANCE));

    REQUIRE(
        driver.frameCount() ==
        2);

    driver.reset();

    REQUIRE_FALSE(
        driver.initialized());

    REQUIRE(
        driver.frameCount() ==
        0);

    REQUIRE(
        driver.lastStatus().clear());

    REQUIRE(
        driver.lastError() ==
        Amg8833DriverError::None);

    REQUIRE(driver.initialize());

    setUniformPixels(
        bus,
        40.0f);

    const Amg8833Acquisition fresh =
        driver.readFrame(
            2000);

    REQUIRE(fresh.success());

    REQUIRE(
        fresh.frame.frameNumber() ==
        1);

    REQUIRE_THAT(
        fresh.frame.pixel(0, 0),
        WithinAbs(
            40.0f,
            TEST_TOLERANCE));
}