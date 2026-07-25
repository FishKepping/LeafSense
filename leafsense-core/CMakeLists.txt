cmake_minimum_required(VERSION 3.20)

project(
    leafsense_core
    VERSION 0.8.0
    LANGUAGES CXX
)

include(FetchContent)

set(
    CMAKE_CXX_STANDARD
    17
)

set(
    CMAKE_CXX_STANDARD_REQUIRED
    ON
)

set(
    CMAKE_CXX_EXTENSIONS
    OFF
)

add_library(
    leafsense_core
    src/thermal_frame.cpp
    src/frame_statistics.cpp
    src/roi/pixel_selection.cpp
    src/roi/rectangle_roi.cpp
    src/roi/polygon_roi.cpp
    src/roi/threshold_roi.cpp
    src/filters/mean_filter.cpp
    src/filters/median_filter.cpp
)

add_library(
    leafsense::core
    ALIAS
    leafsense_core
)

target_include_directories(
    leafsense_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
    leafsense_core
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_options(
        leafsense_core
        PRIVATE
            /W4
            /permissive-
    )
else()
    target_compile_options(
        leafsense_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

option(
    LEAFSENSE_BUILD_TESTS
    "Build LeafSense unit tests"
    ON
)

if(LEAFSENSE_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY
            https://github.com/catchorg/Catch2.git
        GIT_TAG
            v3.7.1
    )

    FetchContent_MakeAvailable(Catch2)

    include(Catch)

    add_executable(
        thermal_frame_tests
        tests/thermal_frame_test.cpp
    )

    target_link_libraries(
        thermal_frame_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        thermal_frame_tests
    )

    add_executable(
        pixel_selection_tests
        tests/pixel_selection_test.cpp
    )

    target_link_libraries(
        pixel_selection_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        pixel_selection_tests
    )

    add_executable(
        frame_statistics_tests
        tests/frame_statistics_test.cpp
    )

    target_link_libraries(
        frame_statistics_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        frame_statistics_tests
    )

    add_executable(
        rectangle_roi_tests
        tests/rectangle_roi_test.cpp
    )

    target_link_libraries(
        rectangle_roi_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        rectangle_roi_tests
    )

    add_executable(
        polygon_roi_tests
        tests/polygon_roi_test.cpp
    )

    target_link_libraries(
        polygon_roi_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        polygon_roi_tests
    )

    add_executable(
        threshold_roi_tests
        tests/threshold_roi_test.cpp
    )

    target_link_libraries(
        threshold_roi_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        threshold_roi_tests
    )

    add_executable(
        mean_filter_tests
        tests/mean_filter_test.cpp
    )

    target_link_libraries(
        mean_filter_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        mean_filter_tests
    )

    add_executable(
        median_filter_tests
        tests/median_filter_test.cpp
    )

    target_link_libraries(
        median_filter_tests
        PRIVATE
            leafsense::core
            Catch2::Catch2WithMain
    )

    catch_discover_tests(
        median_filter_tests
    )
endif()