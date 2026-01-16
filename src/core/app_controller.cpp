#include "app_controller.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

AppController::AppController(QObject *parent)
    : QObject(parent),
      mqtt(nullptr),
      simulationTime(100), // Default 100ms update interval
      lastMoisture(0.0),
      lastTemp(0.0),
      lastHumidity(0.0)
{
    // Create MQTT client instance
    mqtt = new MqttClientQt(this);
}



void AppController::startupMessage()
{
    qDebug() << "Smart irrigation app has started! (Remote Control Mode)";
}

void AppController::startSimulation()
{
    emit simulationStarted();
    sendCommand("START");
}

void AppController::stopSimulation()
{
    sendCommand("STOP");
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

void AppController::onMqttMessageReceived(const QString &topic, const QString &payload)
{
    if (topic == "irrigation/status") {
        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            
            // s: state, m: moisture, t: temp, h: humidity, p: pump, r: rain
            if (obj.contains("m")) onMoistureUpdate(obj["m"].toDouble());
            if (obj.contains("t")) onTempUpdate(obj["t"].toDouble());
            if (obj.contains("h")) onHumidityUpdate(obj["h"].toDouble());
            if (obj.contains("r")) emit rainStatusChanged(obj["r"].toInt() == 1);
            
            // Log full status for debug
            qDebug() << "MQTT Status - State:" << obj["s"].toInt() 
                     << "Moisture:" << obj["m"].toDouble()
                     << "Pump:" << obj["p"].toInt()
                     << "Rain:" << obj["r"].toInt();
        }
    }
}

void AppController::connectToPi(const QString& ip, int port)
{
    if (mqtt) {
        connect(mqtt, &MqttClientQt::messageReceived, 
                this, &AppController::onMqttMessageReceived);
        mqtt->connectToHost(ip, port);
    }
}

void AppController::sendCommand(const QString& cmd)
{
    if (mqtt) {
        mqtt->publish("irrigation/command", cmd);
        qDebug() << "Sent command:" << cmd;
    }
}