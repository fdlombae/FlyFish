cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("FlyFish")

add_executable (FlyFish_Test "src/FlyFish.cpp" "tests/test_main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET FlyFish_Test PROPERTY CXX_STANDARD 20)
endif()

# Testing
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

target_link_libraries(FlyFish_Test PRIVATE gtest gtest_main)

add_custom_command(TARGET FlyFish_Test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:gtest>/gtest.lib"
    "$<TARGET_FILE_DIR:FlyFish_Test>/"
)

