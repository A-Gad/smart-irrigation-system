#include "state_machine.hpp"
#include "logger.hpp"
#include "irrigation_logic.hpp"

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

sensorReading StateMachine::createReading(double moisture) {
    return sensorReading{
        moisture,
        std::chrono::steady_clock::now(),
        IrrigarionLogic::isReadingValid(moisture)
    };
}

void StateMachine::addSensorReading(double moisture)
{
    std::lock_guard<std::mutex> lock(readingsMutex);

    recentReadings.push_back(createReading(moisture));

    if(recentReadings.size() > 10)
    recentReadings.pop_front();
}

SystemState StateMachine::IdleState()
{
    // Perform system health checks
    double moisture = sensor->getMoisture();
    double temp = sensor->getTemp();
    double humid = sensor->getHumid();
    bool isRaining = sensor->isRainDetected();
    bool isHealthy = sensor->isHealthy();
    
    //Validate all sensor readings
    if (!isHealthy) {
        consecutiveReadFailures++;
        spdlog::error("Sensor health check failed in IDLE state (failures: {})", 
                      consecutiveReadFailures);
        
        if (consecutiveReadFailures >= 3) {
            spdlog::error("Multiple sensor failures - entering ERROR state");
            return SystemState::ERROR;
        }
    } else {
        consecutiveReadFailures = 0;  // Reset on successful health check
    }
    
    //Store sensor reading for baseline data
    addSensorReading(moisture);
    
    // Log system status periodically (every 5 minutes)
    auto idleDuration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - stateEntryTime
    );
    
    if (idleDuration.count() % 300 == 0 && idleDuration.count() > 0) {
        spdlog::info("IDLE status check - Moisture: {}%, Temp: {}°C, Humidity: {}%, Rain: {}", 
                     moisture, temp, humid, isRaining ? "YES" : "NO");
    }
    
    //Ensure pump is off in IDLE
    if (pump->isActive()) {
        spdlog::warn("Pump was running in IDLE state - stopping for safety");
        pump->deactivate();
    }
    
    // Check environmental conditions
    if (isRaining) {
        spdlog::debug("Rain detected - remaining in IDLE");
    }
    
    // Auto-transition to MONITORING after stability period
    // This allows the system to self-start after initialization
    auto currentConfig = getConfig();
    if (idleDuration.count() >= 30) {
        // 30 seconds of stable IDLE before auto-starting
        if (isHealthy && !isRaining) {
            spdlog::info("Auto-starting monitoring after stable IDLE period");
            return SystemState::MONITORING;
        }
    }
    
    return SystemState::IDLE;
}
SystemState StateMachine::MonitoringState()
{
    //add sensor reading
    double moisture = sensor->getMoisture();
    addSensorReading(moisture);
    //get filtered moisture from irrigation logic
    double filterdMoisture;
    {
        std::lock_guard<std::mutex> lock(readingsMutex);
        filterdMoisture = IrrigarionLogic::getFilteredMoisture(recentReadings);
    }
    //check for invalid reading
    if(!IrrigarionLogic::isReadingValid(moisture))
    {
        consecutiveReadFailures++;
        spdlog::warn("Invalid sensor reading {} ",moisture);

        if (consecutiveReadFailures >= 3)
        {
            spdlog::error("Multiple sensor failures detected");
            return SystemState::ERROR;
        }
        return SystemState::MONITORING;
    }
    consecutiveReadFailures = 0;

    //check for low moisture
    auto currentConfig = getConfig();

    if (filterdMoisture < currentConfig.lowMoistureThreshold)
    {
        consecutiveLowReadings++;
        spdlog::info("low moisture reading {} (threshold is {})", filterdMoisture, currentConfig.lowMoistureThreshold);
    }
    else{
        consecutiveLowReadings = 0; //reset 
    }

    //check logic to decide if watering is needed
    auto timeSinceLastWatering = std::chrono::duration_cast<std::chrono::minutes>(
        std::chrono::steady_clock::now() - lastWateringTime
    );

    bool shouldWater = IrrigarionLogic::shouldStartWatering(
        filterdMoisture,
        currentConfig.lowMoistureThreshold,
        consecutiveLowReadings,
        timeSinceLastWatering,
        currentConfig.minWateringIntervalMinutes
    );

    if (shouldWater) {
        spdlog::info("Starting watering cycle - Moisture: {}%",filterdMoisture);
        wateringStartTime = std::chrono::steady_clock::now();
        return SystemState::WATERING;
    }

    return SystemState::MONITORING;
}
SystemState StateMachine::WateringState()
{
    //check if its raining
    if(sensor->isRainDetected())
    {
        spdlog::info("Rain Detected, pump will stop");
        pump->deactivate();
    }
    //start pump
    if (!pump->isActive())
    {
        pump->activate();
        spdlog::info("pump started");
    }
    
    //get moisture 
    double moisture = sensor->getMoisture();
    addSensorReading(moisture);
    //get filtered readings
    double filteredMoisture;
    std::optional<double> changeRate;
    {
        std::lock_guard<std::mutex> lock(readingsMutex);
        filteredMoisture = IrrigarionLogic::getFilteredMoisture(recentReadings);
        changeRate = IrrigarionLogic::getMoistuerChangeRate(recentReadings);
    }
    //calculate watering duration
    auto wateringDuration = std::chrono::duration_cast<std::chrono::seconds> (
        std::chrono::steady_clock::now() - wateringStartTime
    );
    auto currentConfig = getConfig();
    //check if we should stop watering 
    bool shouldStop = IrrigarionLogic::shouldStopWatering(
        filteredMoisture,
        currentConfig.highMoistureThreshold,
        wateringDuration,
        currentConfig.maxWateringSeconds,
        changeRate
    );

    if(shouldStop)
    {
        pump->deactivate();
        lastWateringTime = std::chrono::steady_clock::now();
        if (filteredMoisture >= currentConfig.highMoistureThreshold)
        {
            spdlog::info("Target moisture reached: {}%", filteredMoisture);
            return SystemState::WAITING;
        }
        else if (wateringDuration.count() >= currentConfig.maxWateringSeconds) {
            spdlog::warn("Max watering time exceeded");
            return SystemState::ERROR;
        }
        else if (changeRate.has_value() && changeRate.value() < 0.5) {
            spdlog::error("Moisture not increasing - possible pump failure");
            return SystemState::ERROR;
        }
    return SystemState::WAITING;
    }
    if (wateringDuration.count() % 30 == 0) {  // Every 30 seconds
        spdlog::info("Watering progress: {}% (target: {}%, duration: {}s)",
                     filteredMoisture, 
                     currentConfig.highMoistureThreshold,
                     wateringDuration.count());
    }
    return SystemState::WATERING;
}

SystemState StateMachine::WaitingState()
{
    //calculate waite time
    auto waitDuration = std::chrono::duration_cast<std::chrono::minutes>(
        std::chrono::steady_clock::now() - stateEntryTime
    );

    //check if wait period is complete 
    auto currentConfig = getConfig();
    bool shouldResume = IrrigarionLogic::shouldResumeMonitoring(
        waitDuration,
        currentConfig.waitMinutes
    );
    if (shouldResume)
    {
        spdlog::info("wait period complete, resumming ");
        consecutiveLowReadings = 0;
        return SystemState::MONITORING;
    }
    // Log wait progress periodically
    if (waitDuration.count() % 5 == 0)// Every 5 minutes
    {
        spdlog::debug("waiting: {} /{} Minutes",
            waitDuration.count(),
            currentConfig.waitMinutes
        );
    }
    return SystemState::WAITING;
}
SystemState StateMachine::ErrorState()
{
    //stop pump
    if(pump->isActive())
    {
        pump->deactivate();
        spdlog::error("Emergency pump shutdown, an error occured");
    }
    //calculate Error duration
    auto errorDuration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - stateEntryTime
    );
    //check if we can recover
    double moisture = sensor->getMoisture();
    bool lastReadingValid = IrrigarionLogic::isReadingValid(moisture);
    
    auto currentConfig = getConfig();
    int recoveryIntervalSeconds = 300;  // 5 minutes 
    
    bool canRecover = IrrigarionLogic::canRecoverFromError(
        consecutiveReadFailures,
        errorDuration,
        recoveryIntervalSeconds,
        lastReadingValid
    );
    
    if (canRecover) {
        spdlog::info("System recovered from error, resuming monitoring");
        consecutiveReadFailures = 0;
        consecutiveLowReadings = 0;
        return SystemState::MONITORING;
    }
    
    // Log error status periodically
    if (errorDuration.count() % 60 == 0) {  // Every minute
        spdlog::error("System in ERROR state: failures={}, duration={}s, sensor_valid={}",
                      consecutiveReadFailures,
                      errorDuration.count(),
                      lastReadingValid);
    }
    
    return SystemState::ERROR;
}
SystemState StateMachine::ManualOverride()
{
    //Read current sensor state for monitoring
    double moisture = sensor->getMoisture();
    double temp = sensor->getTemp();
    bool isHealthy = sensor->isHealthy();
    
    //Safety check - monitor sensor health 
    if (!isHealthy) {
        spdlog::error("Sensor failure detected in MANUAL mode");
        
        // Safety: Stop pump if sensors fail
        if (pump->isActive()) {
            pump->deactivate();
            spdlog::warn("Pump stopped due to sensor failure in MANUAL mode");
        }
    }
    
    //Store readings for continuity when returning to AUTO
    addSensorReading(moisture);
    
    //Log manual operation status periodically
    auto manualDuration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - stateEntryTime
    );
    
    if (manualDuration.count() % 60 == 0 && manualDuration.count() > 0) {
        spdlog::info("MANUAL mode active for {}s - Pump: {}, Moisture: {}%",
                     manualDuration.count(),
                     pump->isActive() ? "ON" : "OFF",
                     moisture);
    }
    
    // Safety timeout - prevent indefinite manual watering
    auto currentConfig = getConfig();
    int manualTimeoutSeconds = 3600;  //(1 hour)
    
    if (pump->isActive() && manualDuration.count() >= manualTimeoutSeconds) {
        spdlog::warn("Manual watering timeout exceeded ({}s) - stopping pump for safety",
                     manualTimeoutSeconds);
        pump->deactivate();
    }
    
    //Emergency moisture limit - prevent over-watering
    if (pump->isActive() && moisture >= 95.0) {
        spdlog::error("Critical moisture level reached ({}%) - emergency pump stop",
                      moisture);
        pump->deactivate();
    }
    
    //Check for excessive manual duration without pump activity
    if (!pump->isActive() && manualDuration.count() >= (manualTimeoutSeconds * 2)) {
        spdlog::info("Extended MANUAL mode with no activity , returning to AUTO");
        return SystemState::MONITORING;
    }
    // Must always return MANUAL
    // (Automatic transitions are disabled - only commands can exit)
    return SystemState::MANUAL;
}