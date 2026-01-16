#ifndef SIMULATED_HARDWARE_HPP
#define SIMULATED_HARDWARE_HPP

#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"
#include <random>
#include <chrono>

class SimulatedHardware : public ISensorInterface, public IPumpInterface {
public:
    SimulatedHardware();
    ~SimulatedHardware() override = default;

    // ISensorInterface implementation
    bool initialize() override;
    double getMoisture() override;
    double getTemp() override;
    double getHumid() override;
    bool isRainDetected() override;
    bool isHealthy() override;

    // IPumpInterface implementation
    void activate() override;
    void deactivate() override;
    bool isActive() override;

    // Simulation control
    void update(); // Call this periodically to advance simulation physics
    void setRain(bool raining, double intensity);
    
    enum class Scenario { DRY, WET, NORMAL };
    void setScenario(Scenario scenario);

private:
    // Simulation state
    double moistureLevel;
    double actualMoistureLevel;
    double humidity;
    double temperature;
    
    bool isRaining;
    double rainIntensity;
    
    bool pumpRunning;
    bool systemHealthy;
    bool scenarioActive; // Lock temp/humidity when scenario is applied

    // Time tracking
    std::chrono::steady_clock::time_point lastUpdateTime;

    // Random number generation
    std::default_random_engine rng;

    // Simulation helpers
    void updateSensors(double deltaTime);
    double calculateTemperature(int hourOfDay);
    double calculateHumidity(int hourOfDay);
    
    // Constants
    static constexpr double MIN_MOISTURE = 200.0;
    static constexpr double MAX_MOISTURE = 800.0;
};

#endif // SIMULATED_HARDWARE_HPP
