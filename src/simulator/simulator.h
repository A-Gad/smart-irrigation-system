#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>
#include <random>
#include <cmath>

class Simulator : public QObject
{
    Q_OBJECT
public:
    explicit Simulator(QObject* parent = nullptr);
    ~Simulator();

    void startSimulation(int intervalMs);
    void stopSimulation();
    void setPumpRunning(bool running);

signals:
    void soilMoistureUpdated(double value);
    void temperatureUpdated(double value);
    void humidityUpdated(double value);
    void rainDetected();

public slots:
    void deduceRain(int durationSeconds, double intensity);
    // double onSoilMoistureUpdated() const;
    // bool onIsRainDetected() const;
    // double onHumidityUpdated() const;
    // double onTemperatureUpdated() const;

private slots:
    void updateSensors();

private:
    void soilMoisture(double deltaTime, int hourOfDay, double temperature, double humidity);
    void updateRain(double deltaTime);
    double getCurrentTemp(int hourOfDay);
    double getCurrentHumidity(int hourOfDay);

private:
    QTimer* m_updateTimer;
    QElapsedTimer m_elapsedTimer;
    qint64 m_lastUpdateTime;

    std::default_random_engine m_rng;

    double moistureLevel;
    double actualMoistureLevel;
    double humidity;

    bool isRaining;
    int rainingTime; // seconds remaining
    double rainIntensity;
    bool pumpRunning;
};

#endif
