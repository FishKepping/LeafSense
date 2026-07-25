cmake_minimum_required(VERSION 3.20)

project(
    LeafSense
    LANGUAGES CXX
)

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

option(
    LEAFSENSE_BUILD_TESTS
    "Build LeafSense unit tests"
    ON
)

if(LEAFSENSE_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(
    leafsense-core
)

add_subdirectory(
    drivers/amg8833
)