cmake_minimum_required(VERSION 3.13)
project(smart_irrigation_pi LANGUAGES C CXX)

# Basic settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Collect sources in this directory and a possible src/ subdirectory
file(GLOB_RECURSE PI_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(NOT PI_SOURCES)
    message(WARNING "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}. Add sources or adjust file(GLOB...).")
endif()

# Target name (change if you prefer another executable/library name)
set(TARGET_NAME smart_irrigation_pi)

# Build an executable if sources exist
if(PI_SOURCES)
    add_executable(${TARGET_NAME} ${PI_SOURCES})

    # Public include directory convention
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # Recommended warnings and optimizations
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
        target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Release>:-O3> $<$<CONFIG:Debug>:-Og -g>)
    endif()

    # Thread support
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)

    # Try to find common Raspberry Pi GPIO libraries (optional)
    find_library(WIRINGPI_LIB NAMES wiringPi)
    if(WIRINGPI_LIB)
        message(STATUS "Found wiringPi: ${WIRINGPI_LIB}")
        target_compile_definitions(${TARGET_NAME} PRIVATE HAVE_WIRINGPI=1)
        target_link_libraries(${TARGET_NAME} PRIVATE ${WIRINGPI_LIB})
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(PIGPIO QUIET pigpio)
            if(PIGPIO_FOUND)
                message(STATUS "Found pigpio via pkg-config")
                target_compile_definitions(${TARGET_NAME} PRIVATE HAVE_PIGPIO=1)
                target_include_directories(${TARGET_NAME} PRIVATE ${PIGPIO_INCLUDE_DIRS})
                target_link_libraries(${TARGET_NAME} PRIVATE ${PIGPIO_LIBRARIES})
            endif()
        endif()
    endif()

    # Define a macro indicating target is for Pi (overrideable by user)
    if(NOT DEFINED SMART_IRRIGATION_TARGET_PI)
        target_compile_definitions(${TARGET_NAME} PRIVATE SMART_IRRIGATION_TARGET_PI=1)
    endif()

    # Optional install rules
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
    )

else()
    # Create an empty dummy target to avoid configure errors in some CI setups
    add_library(${TARGET_NAME}_dummy INTERFACE)
    target_compile_definitions(${TARGET_NAME}_dummy INTERFACE SMART_IRRIGATION_PI_NO_SOURCES=1)
endif()

# Helpful cache options for consumers
option(SMART_IRRIGATION_USE_WIRINGPI "Prefer linking wiringPi if available" ON)
option(SMART_IRRIGATION_BUILD_TESTS "Enable building tests (if tests exist)" OFF)

# If project root provides global settings, respect them when this directory is included
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(STATUS "Configured ${PROJECT_NAME} in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()