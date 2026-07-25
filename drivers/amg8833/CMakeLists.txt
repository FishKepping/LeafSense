add_library(
    leafsense_amg8833_driver
    src/amg8833_driver.cpp
    src/amg8833_snapshot.cpp
    src/amg8833_telemetry.cpp
)

add_library(
    leafsense::amg8833_driver
    ALIAS
    leafsense_amg8833_driver
)

target_include_directories(
    leafsense_amg8833_driver
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    leafsense_amg8833_driver
    PUBLIC
        leafsense::core
)

target_compile_features(
    leafsense_amg8833_driver
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_options(
        leafsense_amg8833_driver
        PRIVATE
            /W4
            /permissive-
    )
else()
    target_compile_options(
        leafsense_amg8833_driver
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

if(LEAFSENSE_BUILD_TESTS)
    add_executable(
        amg8833_driver_tests
        tests/amg8833_driver_test.cpp
    )

    target_link_libraries(
        amg8833_driver_tests
        PRIVATE
            leafsense::amg8833_driver
            Catch2::Catch2WithMain
    )

    add_test(
        NAME amg8833_driver_tests
        COMMAND amg8833_driver_tests
    )

    add_executable(
        amg8833_recovery_tests
        tests/amg8833_recovery_test.cpp
    )

    target_link_libraries(
        amg8833_recovery_tests
        PRIVATE
            leafsense::amg8833_driver
            Catch2::Catch2WithMain
    )

    add_test(
        NAME amg8833_recovery_tests
        COMMAND amg8833_recovery_tests
    )

    add_executable(
        amg8833_interrupt_tests
        tests/amg8833_interrupt_test.cpp
    )

    target_link_libraries(
        amg8833_interrupt_tests
        PRIVATE
            leafsense::amg8833_driver
            Catch2::Catch2WithMain
    )

    add_test(
        NAME amg8833_interrupt_tests
        COMMAND amg8833_interrupt_tests
    )

    add_executable(
        amg8833_snapshot_tests
        tests/amg8833_snapshot_test.cpp
    )

    target_link_libraries(
        amg8833_snapshot_tests
        PRIVATE
            leafsense::amg8833_driver
            Catch2::Catch2WithMain
    )

    add_test(
        NAME amg8833_snapshot_tests
        COMMAND amg8833_snapshot_tests
    )

    add_executable(
        amg8833_telemetry_tests
        tests/amg8833_telemetry_test.cpp
    )

    target_link_libraries(
        amg8833_telemetry_tests
        PRIVATE
            leafsense::amg8833_driver
            Catch2::Catch2WithMain
    )

    add_test(
        NAME amg8833_telemetry_tests
        COMMAND amg8833_telemetry_tests
    )
endif()