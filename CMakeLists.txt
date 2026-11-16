cmake_minimum_required(VERSION 3.25)

project(FlyFish VERSION 2.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

###############################################################################
# Header-only library
###############################################################################

add_library(FlyFish INTERFACE)
add_library(FlyFish::FlyFish ALIAS FlyFish)

target_compile_features(FlyFish INTERFACE cxx_std_23)
target_include_directories(FlyFish INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

###############################################################################
# Tests (only built by default when FlyFish is the top-level project, so
# add_subdirectory / FetchContent users don't download googletest)
###############################################################################

option(FLYFISH_BUILD_TESTS "Build the FlyFish unit tests" ${PROJECT_IS_TOP_LEVEL})

if(FLYFISH_BUILD_TESTS)
    enable_testing()

    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(FlyFish_Test tests/test_main.cpp)
    target_link_libraries(FlyFish_Test PRIVATE FlyFish::FlyFish GTest::gtest_main)
    set_target_properties(FlyFish_Test PROPERTIES CXX_EXTENSIONS OFF)

    # Warnings only; the optimization level comes from CMAKE_BUILD_TYPE.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(FlyFish_Test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(FlyFish_Test PRIVATE /W4)
    endif()

    include(GoogleTest)
    gtest_discover_tests(FlyFish_Test)
endif()

###############################################################################
# Install / export, so consumers can use find_package(FlyFish)
###############################################################################

include(CMakePackageConfigHelpers)

set(FLYFISH_CMAKE_DIR ${CMAKE_INSTALL_DATADIR}/cmake/FlyFish)

install(TARGETS FlyFish EXPORT FlyFishTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT FlyFishTargets
    NAMESPACE FlyFish::
    DESTINATION ${FLYFISH_CMAKE_DIR}
)

configure_package_config_file(
    cmake/FlyFishConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/FlyFishConfig.cmake
    INSTALL_DESTINATION ${FLYFISH_CMAKE_DIR}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/FlyFishConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FlyFishConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FlyFishConfigVersion.cmake
    DESTINATION ${FLYFISH_CMAKE_DIR}
)
