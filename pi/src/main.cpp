#include <iostream>
#include <thread>
#include <chrono>

#include "logger.hpp"
#include "hardware_factory.hpp"
#include "state_machine.hpp"
#include "mqtt_handler.hpp"
#include "simulated_hardware.hpp" // For casting if needed to access simulation-specific methods

int main(int argc, char* argv[])
{
    //Initialize Logger
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Starting Smart Irrigation System...");

    // Hardware Setup (Factory)
    // Check command line arguments for simulation flag
    bool useSimulator = true; // Default to simulator for now
    if (argc > 1 && std::string(argv[1]) == "--real") {
        useSimulator = false;
        spdlog::info("Mode: REAL HARDWARE");
    } else {
        spdlog::info("Mode: SIMULATOR");
    }

    auto hardware = HardwareFactory::createHardware(useSimulator);
    
    if (!hardware.sensor->initialize()) {
        spdlog::error("Failed to initialize sensors!");
        return 1;
    }
    if (!hardware.pump->initialize()) {
        spdlog::error("Failed to initialize pump!");
        return 1;
    }

    //Configuration & State Machine
    IrrigationConfig config; // Default config
    StateMachine stateMachine(hardware.sensor.get(), hardware.pump.get(), config);

    //MQTT Setup
    std::string broker = "tcp://localhost:1883";
    std::string clientId = "Pi_Controller";
    MqttHandler mqtt(broker, clientId);

    try {
        mqtt.connect();
    } catch (...) {
        spdlog::warn("Failed to connect to MQTT broker - continuing in offline mode");
    }

    // Wiring MQTT callbacks to StateMachine
    mqtt.setCallback([&stateMachine](std::string topic, std::string payload) {
        spdlog::info("MQTT Command received: {} -> {}", topic, payload);
        
        if (payload == "START") {
            stateMachine.sendCommnd(Command::START_AUTO);
        } else if (payload == "STOP") {
            stateMachine.sendCommnd(Command::EMERGENCY_STOP);
        } else if (payload == "MANUAL_ON") {
            stateMachine.sendCommnd(Command::ENABLE_MANUAL);
        } else if (payload == "MANUAL_OFF") {
            stateMachine.sendCommnd(Command::DISABLE_MANUAL);
        }
    });

    // Main Loop
    spdlog::info("System Initialized. Entering main loop...");
    
    auto lastPublishTime = std::chrono::steady_clock::now();

    while (true) {
        // A. Update Simulation (if using simulator)
        // We need to cast back to SimulatedHardware to access the update() method
        // because it's not part of the standard hardware interfaces.
        if (useSimulator) {
            auto sim = std::static_pointer_cast<SimulatedHardware>(hardware.hardwareInstance);
            if (sim) {
                sim->update();
            }
        }

        // B. Update State Machine
        stateMachine.update();

        // C. Publish Status (every 5 seconds)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPublishTime).count() >= 5) {
            
            // Construct compact JSON status
            // s: state (int), m: moisture, t: temp, h: humidity, p: pump (int)
            std::string status = "{";
            status += "\"s\":" + std::to_string(static_cast<int>(stateMachine.getCurrentState())) + ",";
            status += "\"m\":" + std::to_string(hardware.sensor->getMoisture()) + ",";
            status += "\"t\":" + std::to_string(hardware.sensor->getTemp()) + ",";
            status += "\"h\":" + std::to_string(hardware.sensor->getHumid()) + ",";
            status += "\"p\":" + std::to_string(hardware.pump->isActive() ? 1 : 0) + ",";
            status += "\"r\":" + std::to_string(hardware.sensor->isRainDetected() ? 1 : 0);
            status += "}";

            mqtt.publish("irrigation/status", status);
            lastPublishTime = now;
        }

        // D. Loop Delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    mqtt.disconnect();
    return 0;
}