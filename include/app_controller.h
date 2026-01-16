#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QObject>
#include "mqtt_client_qt.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() = default;
    
    void startupMessage();
    void startSimulation();
    void stopSimulation();
    void sendCommand(const QString& cmd);
    void connectToPi(const QString& ip, int port);
    double getCurrentMoisture() const;
    double getCurrentTemp() const;
    double getCurrentHumidity() const;
    
    MqttClientQt* getMqttClient() const { return mqtt; }
    
private:
    MqttClientQt* mqtt;
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
    void rainDetected(); // Keep for simulation event
    void rainStatusChanged(bool isRaining); // For state update

private slots:
    void onMoistureUpdate(double value);
    void onTempUpdate(double value);
    void onHumidityUpdate(double value);
    void onRainDetected();
    void onMqttMessageReceived(const QString &topic, const QString &payload);
};

#endif // APP_CONTROLLER_H