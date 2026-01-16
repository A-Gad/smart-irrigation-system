#include "real_hardware.hpp"
#include "logger.hpp" // Assuming logger is available via CMake include path

bool RealHardware::initialize() {
    spdlog::info("RealHardware: Initializing GPIO...");
    // TODO: Initialize GPIO pins here
    return true;
}

double RealHardware::getMoisture() {
    // Stub: Return a safe default value
    return 0.0;
}

double RealHardware::getTemp() {
    return 0.0;
}

double RealHardware::getHumid() {
    return 0.0;
}

bool RealHardware::isRainDetected() {
    return false;
}

bool RealHardware::isHealthy() {
    // Stub: Always report healthy for now
    return true;
}

void RealHardware::activate() {
    spdlog::info("RealHardware: Pump ON (Stub)");
    pumpState = true;
    // TODO: Write to GPIO
}

void RealHardware::deactivate() {
    spdlog::info("RealHardware: Pump OFF (Stub)");
    pumpState = false;
    // TODO: Write to GPIO
}

bool RealHardware::isActive() {
    return pumpState;
}
