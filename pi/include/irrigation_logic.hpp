#ifndef IRRIGATION_LOGIC_HPP
#define IRRIGATION_LOGIC_HPP

#include <chrono>
#include <optional>
#include <deque>

struct sensorReading{
    double moisturePercent;
    std::chrono::steady_clock::time_point timeStamp;
    bool isValid;
};

class IrrigarionLogic
{
    private:
        std::deque<sensorReading> sensorReadings;
    public:
        static bool isReadingValid(double moisture);//checks validity of sensor reading
        static double getFilteredMoisture(const std::deque<sensorReading>& readings);//gets average last 5 readings
        static std::optional<double>getMoistuerChangeRate(const std::deque<sensorReading>& readings);//gets changerate of moisture
        //decisions
        static bool shouldStartWatering(double filteredMoisture,
        double threshold,
        int consecutiveLowReadings,
        std::chrono::minutes timeSinceLastWatering,
        int minIntervalMinutes);

        static bool shouldStopWatering(
        double filteredMoisture,
        double targetMoisture,
        std::chrono::seconds wateringDuration,
        int maxWateringSeconds,
        std::optional<double> moistureChangeRate);

        static bool shouldResumeMonitoring(
            std::chrono::minutes waitDuration,
            int configuredWaitMinutes);

        static bool canRecoverFromError(
        int consecutiveFailures,
        std::chrono::seconds errorDuration,
        int recoveryIntervalSeconds,
        bool lastReadingValid);
};

#endif