cmake_minimum_required(VERSION 3.14)
project(IrrigationSystem VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

###########################################
# Dependencies
###########################################

# Find or fetch spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include GoogleTest module for test discovery
include(GoogleTest)

###########################################
# Production Library
###########################################

add_library(irrigation_lib
    src/state_machine.cpp
    src/irrigation_logic.cpp
    src/mqtt_handler.cpp
    src/simulated_hardware.cpp
    src/real_hardware.cpp
)

target_include_directories(irrigation_lib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(irrigation_lib 
    PUBLIC 
        spdlog::spdlog
        paho-mqtt3a # Link against Paho MQTT Async C library
)

# Enable warnings
if(MSVC)
    target_compile_options(irrigation_lib PRIVATE /W4)
else()
    target_compile_options(irrigation_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

###########################################
# Test Executable
###########################################

add_executable(irrigation_tests
    tests/unit/test_irrigation_logic.cpp
    tests/unit/test_state_machine.cpp
    tests/integration/test_watering_cycle.cpp
)

target_include_directories(irrigation_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
)

target_link_libraries(irrigation_tests
    PRIVATE
        irrigation_lib
        GTest::gtest_main
        GTest::gmock
)

# Discover tests automatically
gtest_discover_tests(irrigation_tests)

###########################################
# Optional: Main executable (if you have one)
###########################################

add_executable(irrigation_system src/main.cpp)
target_link_libraries(irrigation_system PRIVATE irrigation_lib)