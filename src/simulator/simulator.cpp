#include "simulator.h"

Simulator::Simulator(QObject *parent)
    : QObject(parent),
      moistureLevel(500.0),
      actualMoistureLevel(500.0),
      humidity(50.0),
      isRaining(false),
      rainingTime(0),
      rainIntensity(0.0),
      pumpRunning(false)
{
    // Seed random generator
    std::random_device rd;
    m_rng = std::default_random_engine(rd());

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(100);
    connect(m_updateTimer, &QTimer::timeout, this, &Simulator::updateSensors);

    m_elapsedTimer.start();
    m_lastUpdateTime = m_elapsedTimer.elapsed();

    m_updateTimer->start();
}

Simulator::~Simulator() = default;

void Simulator::startSimulation(int intervalMs)
{
    m_updateTimer->setInterval(intervalMs);
    if (!m_updateTimer->isActive())
        m_updateTimer->start();
}

void Simulator::setPumpRunning(bool running)
{
    pumpRunning = running;
}

void Simulator::deduceRain(int durationSeconds, double intensity)
{
    rainingTime = durationSeconds;
    rainIntensity = intensity;
    isRaining = true;
    emit rainDetected();
}

void Simulator::updateSensors()
{
    qint64 currentTime = m_elapsedTimer.elapsed();
    double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0;
    m_lastUpdateTime = currentTime;

    QTime timeOfDay = QTime::currentTime();
    int hourOfDay = timeOfDay.hour();

    double temperature = getCurrentTemp(hourOfDay);
    double currentHumidity = getCurrentHumidity(hourOfDay);

    updateRain(deltaTime);
    soilMoisture(deltaTime, hourOfDay, temperature, currentHumidity);
}

void Simulator::updateRain(double deltaTime)
{
    if (isRaining) {
        rainingTime -= deltaTime;
        if (rainingTime <= 0) {
            isRaining = false;
            rainIntensity = 0.0;
        } else {
            // Smooth random variability
            std::normal_distribution<double> intensityNoise(0.0, 1.0);
            rainIntensity += intensityNoise(m_rng) * deltaTime;
            rainIntensity = qBound(5.0, rainIntensity, 25.0);
        }
    }
}

double Simulator::getCurrentTemp(int hourOfDay)
{
    // Sinusoidal temperature pattern (peaks at 3 PM)
    double baseTemp = 25.0;
    double amplitude = 8.0;
    double temp = baseTemp + amplitude * std::sin(((hourOfDay - 6) / 12.0) * M_PI);
    emit temperatureUpdated(temp);
    return temp;
}

double Simulator::getCurrentHumidity(int hourOfDay)
{
    // Simple sinusoidal humidity pattern (min at 3 PM, max at 3 AM)
    double baseHumidity = 50.0;
    double amplitude = 20.0;
    double hum = baseHumidity + amplitude * std::cos(((hourOfDay - 6) / 12.0) * M_PI);
    hum = qBound(0.0, hum, 100.0);
    humidity = hum;
    emit humidityUpdated(humidity);
    return humidity;
}

void Simulator::soilMoisture(double deltaTime, int hourOfDay, double temperature, double humidity)
{
    const double minValue = 200.0;
    const double maxValue = 800.0;
    const double fieldCapacity = 700.0;

    // Actual soil saturation
    double actualSaturation = (actualMoistureLevel - minValue) / (maxValue - minValue);
    double sensorSaturation = (moistureLevel - minValue) / (maxValue - minValue);

    // Evaporation based on temperature, time of day, humidity
    double timeMultiplier = (hourOfDay >= 6 && hourOfDay <= 18)
                            ? 0.3 + 0.7 * std::sin(((hourOfDay - 6) / 12.0) * M_PI)
                            : 0.15;
    double tempMultiplier = std::pow(1.07, (temperature - 20.0));
    tempMultiplier = qBound(0.1, tempMultiplier, 3.0);

    double humidityMultiplier = 1.0 - (humidity / 100.0); // more humidity -> less evaporation
    double baseEvaporation = 2.5;
    double evaporation = baseEvaporation * actualSaturation * timeMultiplier * tempMultiplier * humidityMultiplier * deltaTime;

    // Water input from pump and rain
    double waterInput = 0.0;

    if (pumpRunning) {
        double absorptionRate = 1.0 - std::pow(actualSaturation, 2.0);
        waterInput += 8.0 * absorptionRate * deltaTime;
    }

    if (isRaining) {
        double absorptionRate = 1.0 - std::pow(actualSaturation, 1.5);
        waterInput += rainIntensity * absorptionRate * deltaTime;
    }

    // Sensor lag and noise
    const double sensorResponseTime = 5.0;
    double alpha = 1.0 - std::exp(-deltaTime / sensorResponseTime);
    std::normal_distribution<double> noise(0.0, 1.2);
    double sensorNoise = noise(m_rng);

    // Update actual moisture
    actualMoistureLevel += waterInput - evaporation;
    actualMoistureLevel = qBound(minValue, actualMoistureLevel, maxValue);

    // Apply sensor lag
    moistureLevel += alpha * (actualMoistureLevel - moistureLevel) + sensorNoise;
    moistureLevel = qBound(minValue, moistureLevel, maxValue);

    emit soilMoistureUpdated(moistureLevel);
}
