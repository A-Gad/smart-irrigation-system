#include "simulated_hardware.hpp"
#include <cmath>
#include <algorithm>
#include <ctime>

SimulatedHardware::SimulatedHardware()
    : moistureLevel(500.0),
      actualMoistureLevel(500.0),
      humidity(50.0),
      temperature(25.0),
      isRaining(false),
      rainIntensity(0.0),
      pumpRunning(false),
      systemHealthy(true)
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

    temperature = calculateTemperature(hourOfDay);
    humidity = calculateHumidity(hourOfDay);

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
        waterInput += 25.0 * absorptionRate * deltaTime; // Increased pump rate for faster feedback
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
