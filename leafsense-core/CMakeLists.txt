cmake_minimum_required(VERSION 3.16)
project(leafsense_core LANGUAGES CXX)

add_library(leafsense_core STATIC
    src/thermal_frame.cpp
)

target_include_directories(leafsense_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
