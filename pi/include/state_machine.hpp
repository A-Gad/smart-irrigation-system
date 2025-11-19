#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "i_sensor_interface.hpp"
#include "i_pump_interface.hpp"
#include <map>
#include <chrono>

enum class SystemState
{
    IDLE,
    MONITORING,
    WATERING,
    WAITING,
    ERROR,
    MANUAL
};

class StateMachine 
{
    public:
        StateMachine(ISensorInterface* sensor, IPumpInterface* pump);

        void update();//main method to control handlers 
    private:
        ISensorInterface* sensor;
        IPumpInterface* pump;
        //thresholds
        double lowMoistureThreshold;
        double highMoistureThreshold;

        //time stamps
        std::chrono::steady_clock::time_point stateEntryTime;
        std::chrono::steady_clock::time_point pumpStartTime;


        SystemState currentState = SystemState::IDLE; //current System State IDLE as default
        bool requestManual;
        // creating state handlers
        using StateHandler = SystemState (StateMachine::*)(StateMachine&);
        //map of handlers 
        std::map<SystemState,StateHandler> stateHandlers;
        void initHandlers();

        //a function for each state
        SystemState IdleState(StateMachine&);
        SystemState MonitoringState(StateMachine&);
        SystemState WateringState(StateMachine&);
        SystemState WaitingState(StateMachine&);
        SystemState ErrorState(StateMachine&);
        SystemState ManualOverride(StateMachine&);
};

#endif