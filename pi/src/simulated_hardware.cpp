#include "simulated_hardware.hpp"
#include <cmath>
#include <algorithm>
#include <ctime>
#include <spdlog/spdlog.h>

SimulatedHardware::SimulatedHardware()
    : moistureLevel(500.0),
      actualMoistureLevel(500.0),
      humidity(50.0),
      temperature(25.0),
      isRaining(false),
      rainIntensity(0.0),
      pumpRunning(false),
      systemHealthy(true),
      scenarioActive(false)
{
    std::random_device rd;
    rng = std::default_random_engine(rd());
    lastUpdateTime = std::chrono::steady_clock::now();
}

bool SimulatedHardware::initialize() {
    lastUpdateTime = std::chrono::steady_clock::now();
    return true;
}

// Sensor Interface Implementation
double SimulatedHardware::getMoisture() {
    // Return scaled percentage (0-100) based on raw value
    // Assuming 200 is 0% and 800 is 100% based on simulator logic
    double percentage = ((moistureLevel - MIN_MOISTURE) / (MAX_MOISTURE - MIN_MOISTURE)) * 100.0;
    return std::clamp(percentage, 0.0, 100.0);
}

double SimulatedHardware::getTemp() {
    return temperature;
}

double SimulatedHardware::getHumid() {
    return humidity;
}

bool SimulatedHardware::isRainDetected() {
    return isRaining;
}

bool SimulatedHardware::isHealthy() {
    return systemHealthy;
}

// Pump Interface Implementation
void SimulatedHardware::activate() {
    pumpRunning = true;
}

void SimulatedHardware::deactivate() {
    pumpRunning = false;
}

bool SimulatedHardware::isActive() {
    return pumpRunning;
}

void SimulatedHardware::setRain(bool raining, double intensity) {
    isRaining = raining;
    rainIntensity = intensity;
}

void SimulatedHardware::update() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> distinct = now - lastUpdateTime;
    double deltaTime = distinct.count();
    lastUpdateTime = now;

    updateSensors(deltaTime);
}

void SimulatedHardware::updateSensors(double deltaTime) {
    // Get current hour for environmental cycles
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    int hourOfDay = now->tm_hour;

    // Only recalculate temp/humidity if no scenario is active
    if (!scenarioActive) {
        temperature = calculateTemperature(hourOfDay);
        humidity = calculateHumidity(hourOfDay);
    }

    // Soil Moisture Physics

    // Saturation ratios (0.0 to 1.0)
    double actualSaturation = (actualMoistureLevel - MIN_MOISTURE) / (MAX_MOISTURE - MIN_MOISTURE);
    
    // Evaporation factors
    double timeMultiplier = (hourOfDay >= 6 && hourOfDay <= 18)
                            ? 0.3 + 0.7 * std::sin(((hourOfDay - 6) / 12.0) * M_PI)
                            : 0.15;
    
    double tempMultiplier = std::pow(1.07, (temperature - 20.0));
    tempMultiplier = std::clamp(tempMultiplier, 0.1, 3.0);

    double humidityMultiplier = 1.0 - (humidity / 100.0);
    double baseEvaporation = 2.5; // Units per second
    
    double evaporation = baseEvaporation * actualSaturation * timeMultiplier * tempMultiplier * humidityMultiplier * deltaTime;

    // Water Input
    double waterInput = 0.0;

    if (pumpRunning) {
        double absorptionRate = 1.0 - std::pow(actualSaturation, 2.0);
        waterInput += 150.0 * absorptionRate * deltaTime; // Boosted pump rate (was 25.0)
    }

    if (isRaining) {
        double absorptionRate = 1.0 - std::pow(actualSaturation, 1.5);
        waterInput += (rainIntensity > 0 ? rainIntensity : 5.0) * absorptionRate * deltaTime;
    }

    // Update Actual Moisture
    actualMoistureLevel += waterInput - evaporation;
    actualMoistureLevel = std::clamp(actualMoistureLevel, MIN_MOISTURE, MAX_MOISTURE);

    // Sensor Lag and Noise
    const double sensorResponseTime = 2.0; // Faster response for testing
    double alpha = 1.0 - std::exp(-deltaTime / sensorResponseTime);
    
    std::normal_distribution<double> noiseDist(0.0, 0.5); // Reduced noise
    double noise = noiseDist(rng);

    moistureLevel += alpha * (actualMoistureLevel - moistureLevel) + noise;
    moistureLevel = std::clamp(moistureLevel, MIN_MOISTURE, MAX_MOISTURE);

    static int logCounter = 0;
    if (++logCounter >= 200) { // Log every ~20 seconds (assuming 100ms update rate, wait, main loop is 100ms? update() called every loop? yes. so 10 calls/sec. 200 calls = 20s. Maybe too slow? let's do 20 calls = 2s)
       // Update: use time-based logging
    }
    
    // Better: use static timer
    static auto lastLog = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastLog).count() >= 1) {
        spdlog::info("PHYSICS: Moisture={:.1f} (Target={:.1f}), Pump={}, Rain={}, Input={:.2f}, Evap={:.2f}, dT={:.3f}",
            moistureLevel, actualMoistureLevel, pumpRunning, isRaining, waterInput, evaporation, deltaTime);
        lastLog = std::chrono::steady_clock::now();
    }
}

double SimulatedHardware::calculateTemperature(int hourOfDay) {
    double baseTemp = 25.0;
    double amplitude = 8.0;
    // Peak at 3 PM (15:00)
    return baseTemp + amplitude * std::sin(((hourOfDay - 6) / 12.0) * M_PI);
}

double SimulatedHardware::calculateHumidity(int hourOfDay) {
    double baseHumidity = 50.0;
    double amplitude = 20.0;
    // Inverse to temp, min at 3 PM
    double hum = baseHumidity + amplitude * std::cos(((hourOfDay - 6) / 12.0) * M_PI);
    return std::clamp(hum, 0.0, 100.0);
}

void SimulatedHardware::setScenario(Scenario scenario) {
    scenarioActive = true; // Lock temp/humidity at scenario values
    
    if (scenario == Scenario::DRY) {
        actualMoistureLevel = 250.0; // Very dry
        moistureLevel = 250.0;
        temperature = 35.0; // Hot
        humidity = 20.0;
        isRaining = false;
    } else if (scenario == Scenario::WET) {
        actualMoistureLevel = 750.0; // Wet
        moistureLevel = 750.0;
        temperature = 18.0; // Cool
        humidity = 80.0;
        isRaining = true; // Rain
    } else { // NORMAL
        scenarioActive = false; // Release lock for NORMAL
        actualMoistureLevel = 500.0;
        moistureLevel = 500.0;
        temperature = 25.0;
        humidity = 50.0;
        isRaining = false;
        spdlog::info("SCENARIO: NORMAL APPLIED"); 
    }
    
    if (scenario == Scenario::DRY) spdlog::info("SCENARIO: DRY APPLIED");
    if (scenario == Scenario::WET) spdlog::info("SCENARIO: WET APPLIED");

    // Reset lastUpdateTime to prevent huge time jump if system was idle? 
    // Actually, update() handles dT based on wall clock. If we warp values, we don't change time.
    // But let's log the post-set values.
    spdlog::info("SCENARIO RESULT: Moisture set to {}, Scenario Lock: {}", moistureLevel, scenarioActive);
}
