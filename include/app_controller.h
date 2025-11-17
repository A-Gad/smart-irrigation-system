#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include "simulator.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() = default;
    
    void startupMessage();
    void startSimulation();
    void stopSimulation();
    double getCurrentMoisture() const;
    double getCurrentTemp() const;
    double getCurrentHumidity() const;
    
    Simulator* getSimulator() const { return sim; }
    
private:
    Simulator* sim;
    int simulationTime;
    
    // Cache latest values
    double lastMoisture;
    double lastTemp;
    double lastHumidity;

signals:
    void simulationStarted();
    void moistureUpdated(double value);
    void tempUpdated(double value);
    void humidityUpdated(double value);
    void rainDetected();

private slots:
    void onMoistureUpdate(double value);
    void onTempUpdate(double value);
    void onHumidityUpdate(double value);
    void onRainDetected();
};

#endif // APP_CONTROLLER_H