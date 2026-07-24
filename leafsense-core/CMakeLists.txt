cmake_minimum_required(VERSION 3.16)

project(leafsense-core)

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
