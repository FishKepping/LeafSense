cmake_minimum_required(VERSION 3.16)

project(leafsense-core)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

add_library(leafsense-core
    src/thermal_frame.cpp
)

target_include_directories(leafsense-core
    PUBLIC
        include
)

target_compile_features(leafsense-core
    PUBLIC
        cxx_std_17
)

add_executable(thermal_frame_tests
    tests/thermal_frame_test.cpp
)

target_link_libraries(thermal_frame_tests
    PRIVATE
        leafsense-core
        Catch2::Catch2WithMain
)