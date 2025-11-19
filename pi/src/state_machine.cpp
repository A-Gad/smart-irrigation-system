#include "state_machine.hpp"

StateMachine::StateMachine(ISensorInterface* sensor, IPumpInterface* pump)
    :sensor(sensor), pump(pump),
    lowMoistureThreshold(30.0),    
    highMoistureThreshold(60.0),   
    currentState(SystemState::IDLE)
{
    initHandlers();
    stateEntryTime = std::chrono::steady_clock::now();
}

void StateMachine::initHandlers()
{
    stateHandlers[SystemState::IDLE]       = &StateMachine::IdleState;
    stateHandlers[SystemState::MONITORING] = &StateMachine::MonitoringState;
    stateHandlers[SystemState::WATERING]   = &StateMachine::WateringState;
    stateHandlers[SystemState::WAITING]    = &StateMachine::WaitingState;
    stateHandlers[SystemState::ERROR]      = &StateMachine::ErrorState;
    stateHandlers[SystemState::MANUAL]     = &StateMachine::ManualOverride;
}

void StateMachine::update()
{
    // check for manual control request
    if (requestManual && currentState != SystemState::MANUAL) 
    {
        currentState = SystemState::MANUAL;
        stateEntryTime = std::chrono::steady_clock::now();
    }


    auto handler = stateHandlers[currentState];

    SystemState nextState = (this->*handler)(*this);

   if (nextState != currentState) 
    {
        currentState = nextState;
        stateEntryTime = std::chrono::steady_clock::now();
    }
}

