#include "state_machine.hpp"
#include "logger.hpp"

std::string StateMachine::stateToString(SystemState state)
{
    switch(state) {
        case SystemState::IDLE: return "IDLE";
        case SystemState::MONITORING: return "MONITORING";
        case SystemState::WATERING: return "WATERING";
        case SystemState::WAITING: return "WAITING";
        case SystemState::ERROR: return "ERROR";
        case SystemState::MANUAL: return "MANUAL";
    }
    return "UNKNOWN";
}
std::string StateMachine::commandToString(Command cmd)
{
    switch(cmd) {
        case Command::START_AUTO: return "START_AUTO";
        case Command::ENABLE_MANUAL: return "ENABLE_MANUAL";
        case Command::DISABLE_MANUAL: return "DISABLE_MANUAL";
        case Command::EMERGENCY_STOP: return "EMERGENCY_STOP";
    }
    return "UNKNOWN";
}

StateMachine::StateMachine(ISensorInterface* sensor, IPumpInterface* pump, const IrrigationConfig& config)
    :sensor(sensor), pump(pump),
    config(config),
    currentState(SystemState::IDLE)
{
    initHandlers();
    stateEntryTime = std::chrono::steady_clock::now();

    spdlog::info("System started for zone: {}",config.zoneName);
    spdlog::info("Initial state: {}", stateToString(currentState));
    spdlog::info("Soil Type: {}", config.soilType);
    spdlog::info("Thresholds - Low: {}%, High: {}%", config.lowMoistureThreshold, config.highMoistureThreshold);
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

void StateMachine::sendCommnd(Command cmd)
{
    std::lock_guard<std::mutex> lock(commandMutex);
    commands.push(cmd);
    spdlog::debug("Command queued: {}", commandToString(cmd));
}

void StateMachine::processCommand(Command cmd)
{
    spdlog::info("Processing command: {} (current state: {})", 
                 commandToString(cmd), stateToString(currentState));

    switch(cmd) {
        case Command::START_AUTO:
            pendingAction = PendingAction::ENTER_AUTO;
            spdlog::info("Will transition to AUTO mode");
            break;

        case Command::ENABLE_MANUAL:
            pendingAction = PendingAction::ENTER_MANUAL;
            spdlog::warn("MANUAL MODE requested - automatic control disabled");
            break;

        case Command::DISABLE_MANUAL:
            pendingAction = PendingAction::EXIT_MANUAL;
            spdlog::info("→ Exiting MANUAL mode");
            break;

        case Command::EMERGENCY_STOP:
            pendingAction = PendingAction::EMERGENCY_STOP;
            spdlog::error("EMERGENCY STOP activated!");
            break;
    }
}
IrrigationConfig StateMachine::getConfig() const
{
    std::lock_guard<std::mutex> lock(configMutex);
    return config;
}
void StateMachine::updateConfig(const IrrigationConfig& newconfig)
{
    std::lock_guard<std::mutex> lock(configMutex);
    config = newconfig;
    spdlog::info("[{}] Configuration updated: {}% - {}%",
                    config.zoneName,
                    config.lowMoistureThreshold,
                    config.highMoistureThreshold);
}
void StateMachine::update()
{
    {
    std::lock_guard <std::mutex> lock(commandMutex);
    while (!commands.empty())
    {
        Command cmd = commands.front();
        commands.pop();
        processCommand(cmd);
    }
    }

    if (pendingAction != PendingAction::NONE)
    {
        switch (pendingAction)
        {
            case PendingAction::ENTER_AUTO:
                currentState = SystemState::MONITORING;
                break;

            case PendingAction::ENTER_MANUAL:
                currentState = SystemState::MANUAL;
                break;

            case PendingAction::EXIT_MANUAL:
                currentState = SystemState::MONITORING;
                break;

            case PendingAction::EMERGENCY_STOP:
                currentState = SystemState::ERROR;
                break;

            default:
                break;
        }
        pendingAction = PendingAction::NONE;
        stateEntryTime = std::chrono::steady_clock::now();
    }

    if (currentState == SystemState::MANUAL)
    {
        StateHandler handler = stateHandlers.at(currentState);
        (this->*handler)();

        publishedState.store(currentState, std::memory_order_relaxed);

        return;
    }

    auto handler = stateHandlers[currentState];

    SystemState nextState = (this->*handler)();

   if (nextState != currentState) 
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - stateEntryTime);
        
        spdlog::info("STATE CHANGE: {} → {} (after {}s)", 
                    stateToString(currentState), 
                    stateToString(nextState),
                    duration.count());
                    
        currentState = nextState;
        stateEntryTime = std::chrono::steady_clock::now();
    }
    publishedState = currentState;
}

SystemState StateMachine::IdleState()
{
    // ---------------------
    //  Read sensors (optional)
    //  Example: double moisture = sensor->readMoisture();
    // ---------------------
    double moisture = sensor->getMoisture();

    // ---------------------
    //  Your logic here
    //  - do nothing?
    //  - wait for command?
    // ---------------------
    

    // Return next state (or same state)
    return SystemState::IDLE;
}
SystemState StateMachine::MonitoringState()
{
    // ---------------------
    // Read sensor(s)
    // ---------------------
    // double moisture = sensor->readMoisture();


    // ---------------------
    // Add your logic
    // - check moisture threshold
    // - check for sensor faults
    // - check timing conditions
    // ---------------------


    // Example (keep or remove)
    // if (moisture < lowMoistureThreshold)
    //     return SystemState::WATERING;

    return SystemState::MONITORING;
}
SystemState StateMachine::WateringState()
{
    // ---------------------
    // Run pump logic
    // ---------------------
    // pump->start();
    // double moisture = sensor->readMoisture();


    // ---------------------
    // Your logic here
    // - check if moisture reached high threshold
    // - check if max watering time exceeded
    // - check for pump failure
    // ---------------------

    
    return SystemState::WATERING;
}
SystemState StateMachine::WaitingState()
{
    // ---------------------
    // Check elapsed time
    // ---------------------
    // auto now = std::chrono::steady_clock::now();
    // auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stateEntryTime).count();


    // ---------------------
    // Your logic here
    // - wait N minutes
    // - then go back to MONITORING
    // ---------------------


    return SystemState::WAITING;
}
SystemState StateMachine::ErrorState()
{
    // ---------------------
    // Cleanup
    // ---------------------
    // pump->stop();


    // ---------------------
    // Your logic
    // - wait until user resets system
    // - check if hardware recovered
    // ---------------------


    return SystemState::ERROR;
}
SystemState StateMachine::ManualOverride()
{
    // ---------------------
    // Manual control logic
    // ---------------------
    // - read user inputs
    // - turn pump on/off based on manual commands
    // - ignore automatic state transitions
    // ---------------------


    // Must always return MANUAL  
    // (Auto transitions disabled in update())
    return SystemState::MANUAL;
}
