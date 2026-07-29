cmake_minimum_required(VERSION 3.20)

project(
    leafsense_core
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    leafsense_core
    src/thermal_frame.cpp
    src/frame_statistics.cpp
    src/amg8833_decoder.cpp
    src/amg8833_registers.cpp
    src/thermal_processor.cpp
    src/roi/pixel_selection.cpp
    src/roi/rectangle_roi.cpp
    src/roi/polygon_roi.cpp
    src/roi/threshold_roi.cpp
    src/filters/mean_filter.cpp
    src/filters/median_filter.cpp
    src/filters/exponential_filter.cpp
    src/measurement/measurement_channel.cpp
    src/measurement/measurement_channel_manager.cpp
    src/measurement/measurement_channel_controller.cpp
    src/transport/thermal_frame_packet.cpp
    src/transport/thermal_frame_publisher.cpp
    src/rendering/thermal_renderer.cpp
)

add_library(leafsense::core ALIAS leafsense_core)

target_include_directories(
    leafsense_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(leafsense_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(leafsense_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(leafsense_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(LEAFSENSE_BUILD_TESTS "Build LeafSense unit tests" ON)

if(LEAFSENSE_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
    )

    FetchContent_MakeAvailable(Catch2)
    include(Catch)

    function(leafsense_add_test target_name source_file)
        add_executable(${target_name} ${source_file})
        target_link_libraries(
            ${target_name}
            PRIVATE leafsense::core Catch2::Catch2WithMain
        )
        catch_discover_tests(${target_name})
    endfunction()

    leafsense_add_test(thermal_frame_tests tests/thermal_frame_test.cpp)
    leafsense_add_test(pixel_selection_tests tests/pixel_selection_test.cpp)
    leafsense_add_test(frame_statistics_tests tests/frame_statistics_test.cpp)
    leafsense_add_test(rectangle_roi_tests tests/rectangle_roi_test.cpp)
    leafsense_add_test(polygon_roi_tests tests/polygon_roi_test.cpp)
    leafsense_add_test(threshold_roi_tests tests/threshold_roi_test.cpp)
    leafsense_add_test(mean_filter_tests tests/mean_filter_test.cpp)
    leafsense_add_test(median_filter_tests tests/median_filter_test.cpp)
    leafsense_add_test(exponential_filter_tests tests/exponential_filter_test.cpp)
    leafsense_add_test(amg8833_decoder_tests tests/amg8833_decoder_test.cpp)
    leafsense_add_test(thermal_processor_tests tests/thermal_processor_test.cpp)
    leafsense_add_test(amg8833_registers_tests tests/amg8833_registers_test.cpp)
    leafsense_add_test(measurement_channel_tests tests/measurement_channel_test.cpp)
    leafsense_add_test(measurement_channel_controller_tests tests/measurement_channel_controller_test.cpp)
    leafsense_add_test(thermal_frame_packet_tests tests/thermal_frame_packet_test.cpp)
    leafsense_add_test(thermal_renderer_tests tests/thermal_renderer_test.cpp)
endif()
