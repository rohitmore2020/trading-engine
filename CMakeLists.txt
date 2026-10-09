cmake_minimum_required(VERSION 3.22)

project(trading_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Options
option(TE_BUILD_TESTS "Build tests" ON)
option(TE_BUILD_BENCHMARKS "Build benchmarks" OFF)

# Required external packages
find_package(Threads REQUIRED)

# Project modules
include(cmake/Warnings.cmake)
include(cmake/Sanitizers.cmake)

# Core library
add_library(te_core STATIC
    src/placeholder.cpp
    src/validation.cpp
)

target_include_directories(te_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(te_core
    PUBLIC
        cxx_std_20
)

target_link_libraries(te_core
    PUBLIC
        te_warnings
        te_sanitizers
        Threads::Threads
)

# Main executable (thin wrapper)
add_executable(te_engine
    apps/engine_main.cpp
)

target_link_libraries(te_engine
    PRIVATE
        te_core
)

# Testing harness
if(TE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Prevent GoogleTest from installing or overriding global options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()