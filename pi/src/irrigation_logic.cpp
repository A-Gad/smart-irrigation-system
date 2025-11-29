#include "irrigation_logic.hpp"

bool IrrigarionLogic::isReadingValid(double moisture)
{
    return moisture >= -5 && moisture <= 105;
}

double IrrigarionLogic::getFilteredMoisture(const std::deque<sensorReading>& readings)
{
    double sum = 0;
    int count = 0;

    if (readings.empty())
    {
        return 0.0;
    }
    for (auto it = readings.begin(); it != readings.end() && count < 5; ++it)
    {
        sum += it->moisturePercent;
        count ++;
    }
    return(((count > 5)? (sum/count) : readings.back().moisturePercent));
}

std::optional<double> IrrigarionLogic::getMoistuerChangeRate(const std::deque<sensorReading>& readings)
{
    if(readings.size() < 2)
    return std::nullopt;

    auto newest = readings.back();
    auto oldest = readings.front();

    if(!newest.isValid || !oldest.isValid)
    return std::nullopt;

    auto timeDiff = std::chrono::duration_cast<std::chrono::minutes>
    (newest.timeStamp - oldest.timeStamp);

    if (timeDiff.count() == 0)
    return std::nullopt;

    double moistureDiff = newest.moisturePercent - oldest.moisturePercent;

    return moistureDiff/timeDiff.count();
}

bool IrrigarionLogic::shouldStartWatering(
    double filteredMoisture,
    double threshold,
    int consecutiveLowReadings,
    std::chrono::minutes timeSinceLastWatering,
    int minIntervalMinutes)
{
    if(filteredMoisture >= threshold)
    return false;

    if (consecutiveLowReadings < 3)
    return false;

    if (timeSinceLastWatering.count() < minIntervalMinutes)
    return false;

    return true;
}

bool IrrigarionLogic::shouldStopWatering(
        double filteredMoisture,
        double targetMoisture,
        std::chrono::seconds wateringDuration,
        int maxWateringSeconds,
        std::optional<double> moistureChangeRate)
{
    // Success: Target reached
    if (filteredMoisture >= targetMoisture) {
        return true;
    }
        
    // Timeout: Max time exceeded
    if (wateringDuration.count() >= maxWateringSeconds) {
        return true;
    }
        
    // Problem: Moisture not increasing
    if (moistureChangeRate.has_value() &&
        wateringDuration.count() > 10 &&
        moistureChangeRate.value() < 0.5) {
        return true;  // Pump failure?
    }
        
    return false;
}

bool IrrigarionLogic::shouldResumeMonitoring(
    std::chrono::minutes waitDuration,
    int configuredWaitMinutes)
{
    return waitDuration.count() >= configuredWaitMinutes;
}
    
bool IrrigarionLogic::canRecoverFromError(
    int consecutiveFailures,
    std::chrono::seconds errorDuration,
    int recoveryIntervalSeconds,
    bool lastReadingValid)
{
    if (consecutiveFailures > 0) return false;
    if (errorDuration.count() < recoveryIntervalSeconds) return false;
    if (!lastReadingValid) return false;
    return true;
}