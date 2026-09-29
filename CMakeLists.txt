cmake_minimum_required(VERSION 3.25)

# Set C++23 standard for the entire project
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("FlyFish" VERSION 1.1.1 LANGUAGES CXX)

# Enable modern CMake features
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Create executable
add_executable(FlyFish_Test "tests/test_main.cpp")

# Compiler-specific optimizations (applied per-target to avoid affecting dependencies)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FlyFish_Test PRIVATE -Wall -Wextra -Wpedantic -Werror -O3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(FlyFish_Test PRIVATE /W4)
endif()

# Set properties for the target
set_target_properties(FlyFish_Test PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Testing setup
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

target_link_libraries(FlyFish_Test PRIVATE FlyFish gtest gtest_main)

# Header-only library setup
add_library(FlyFish INTERFACE)

target_include_directories(FlyFish INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
