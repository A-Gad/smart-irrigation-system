#include "app_controller.h"
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent),
      sim(nullptr),
      simulationTime(100), // Default 100ms update interval
      lastMoisture(0.0),
      lastTemp(0.0),
      lastHumidity(0.0)
{
    // Create simulator instance
    sim = new Simulator(this);
    
    // Connect simulator signals to controller slots
    connect(sim, &Simulator::soilMoistureUpdated, 
            this, &AppController::onMoistureUpdate);
    connect(sim, &Simulator::temperatureUpdated, 
            this, &AppController::onTempUpdate);
    connect(sim, &Simulator::humidityUpdated, 
            this, &AppController::onHumidityUpdate);
    connect(sim, &Simulator::rainDetected, 
            this, &AppController::onRainDetected);
}



void AppController::startupMessage()
{
    qDebug() << "Smart irrigation app has started!";
}

void AppController::startSimulation()
{
    if (sim) {
        sim->startSimulation(simulationTime);
        emit simulationStarted();
        qDebug() << "Simulation started with interval:" << simulationTime << "ms";
    }
}

void AppController::stopSimulation()
{
    if (sim) {
        sim->stopSimulation();
        qDebug() << "Simulation stopped";
    }
}

double AppController::getCurrentMoisture() const
{
    return lastMoisture;
}

double AppController::getCurrentTemp() const
{
    return lastTemp;
}

double AppController::getCurrentHumidity() const
{
    return lastHumidity;
}

void AppController::onMoistureUpdate(double value)
{
    lastMoisture = value;
    emit moistureUpdated(value);
}

void AppController::onTempUpdate(double value)
{
    lastTemp = value;
    emit tempUpdated(value);
}

void AppController::onHumidityUpdate(double value)
{
    lastHumidity = value;
    emit humidityUpdated(value);
}

void AppController::onRainDetected()
{
    emit rainDetected();
    qDebug() << "Rain detected!";
}