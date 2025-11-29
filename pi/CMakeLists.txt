cmake_minimum_required(VERSION 3.14)
project(IrrigationSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Fetch Google Mock (included in googletest)
enable_testing()

# Production code
add_library(irrigation_lib
    src/state_machine.cpp
    src/irrigation_logic.cpp
)

target_include_directories(irrigation_lib PUBLIC include)

# Link spdlog (assuming you have it)
find_package(spdlog REQUIRED)
target_link_libraries(irrigation_lib PUBLIC spdlog::spdlog)

# Test executable
add_executable(irrigation_tests
    tests/unit/test_irrigation_logic.cpp
    tests/unit/test_state_machine.cpp
    tests/integration/test_watering_cycle.cpp
)

target_link_libraries(irrigation_tests
    irrigation_lib
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(irrigation_tests)