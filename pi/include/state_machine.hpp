#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"
#include "irrigation_logic.hpp"
#include <map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <queue>

enum class SystemState
{
    IDLE,
    MONITORING,
    WATERING,
    WAITING,
    ERROR,
    MANUAL
};
enum class Command
{
    START_AUTO,
    ENABLE_MANUAL,
    DISABLE_MANUAL,
    EMERGENCY_STOP
};

enum class PendingAction {
    NONE,
    ENTER_AUTO,
    ENTER_MANUAL,
    EXIT_MANUAL,
    EMERGENCY_STOP
};


struct IrrigationConfig {
    std::string zoneName = "Main Zone";
    std::string soilType = "";
    double lowMoistureThreshold = 30.0;
    double highMoistureThreshold = 60.0;
    int maxWateringSeconds = 60;
    int waitMinutes = 1; // Reduced for testing (was 15)
    int minWateringIntervalMinutes = 1; // Reduced for testing (was 30)

    //presests:-
    static IrrigationConfig forClay(const std::string& name) {
        return {
            .zoneName = name,
            .soilType = "Clay",
            .lowMoistureThreshold = 40.0,   // Retains water well
            .highMoistureThreshold = 70.0,
            .maxWateringSeconds = 45,
            .waitMinutes = 20,
            .minWateringIntervalMinutes = 45
        };
    }
    static IrrigationConfig forSandy(const std::string& name) {
        return {
            .zoneName = name,
            .soilType = "Sandy",
            .lowMoistureThreshold = 25.0,   // Drains quickly
            .highMoistureThreshold = 55.0,
            .maxWateringSeconds = 30,
            .waitMinutes = 10,
            .minWateringIntervalMinutes = 20
        };
    }
    static IrrigationConfig forLoam(const std::string& name) {
        return {
            .zoneName = name,
            .soilType = "Loam",
            .lowMoistureThreshold = 30.0,   // Balanced
            .highMoistureThreshold = 60.0,
            .maxWateringSeconds = 40,
            .waitMinutes = 15,
            .minWateringIntervalMinutes = 30
        };
    }
    static IrrigationConfig forPeat(const std::string& name) {
        return {
            .zoneName = name,
            .soilType = "Peat",
            .lowMoistureThreshold = 35.0,   // Holds moisture
            .highMoistureThreshold = 65.0,
            .maxWateringSeconds = 35,
            .waitMinutes = 18,
            .minWateringIntervalMinutes = 40
        };
    }
};

class StateMachine 
{
    public:
        StateMachine(ISensorInterface* sensor, IPumpInterface* pump, const IrrigationConfig& config);

        void update();//main method to control handlers 

        void sendCommnd(Command cmd);
        //helper methods
        std::string stateToString(SystemState state);
        std::string commandToString(Command cmd);
        IrrigationConfig getConfig()const; 
        void updateConfig(const IrrigationConfig& newconfig);
        SystemState getCurrentState();
    private:

        std::mutex commandMutex;
        std::queue<Command> commands;
        
        mutable std::mutex configMutex;
        
        ISensorInterface* sensor;       
        IPumpInterface* pump;             
        IrrigationConfig config;
       
        std::deque<sensorReading> recentReadings;
        mutable std::mutex readingsMutex;

        // Error tracking
        int consecutiveReadFailures = 0;
        int consecutiveLowReadings = 0;
        bool pumpIsRunning = false;

        //time stamps
        std::chrono::steady_clock::time_point stateEntryTime;
        std::chrono::steady_clock::time_point pumpStartTime;


        SystemState currentState = SystemState::IDLE; //current System State IDLE as default
        std::atomic<SystemState> publishedState; //atomic for safe reads from other threads

        std::chrono::steady_clock::time_point lastWateringTime;
        std::chrono::steady_clock::time_point wateringStartTime;

        PendingAction pendingAction = PendingAction::NONE;

        // creating state handlers
        using StateHandler = SystemState (StateMachine::*)();
        //map of handlers 
        std::map<SystemState,StateHandler> stateHandlers;
        void initHandlers();

        //a function for each state
        SystemState IdleState();
        SystemState MonitoringState();
        SystemState WateringState();
        SystemState WaitingState();
        SystemState ErrorState();
        SystemState ManualOverride();

        //command processing 
        void processCommand(Command cmd);

        void addSensorReading(double moisture);
        sensorReading createReading(double moisture);
};

#endif