#ifndef MQTT_CLIENT_QT_H
#define MQTT_CLIENT_QT_H

#include <QObject>
#include <QString>
#include <MQTTAsync.h> // Paho MQTT

class MqttClientQt : public QObject {
    Q_OBJECT

public:
    explicit MqttClientQt(QObject *parent = nullptr);
    ~MqttClientQt();

    void connectToHost(const QString &host, int port);
    void disconnectFromHost();

private:
    // Static callbacks for Paho MQTT
    static void onConnectSuccess(void* context, MQTTAsync_successData* response);
    static void onConnectFailure(void* context, MQTTAsync_failureData* response);
    static void onDisconnectSuccess(void* context, MQTTAsync_successData* response);
    static void onSubscribeSuccess(void* context, MQTTAsync_successData* response);
    static void onPublishSuccess(void* context, MQTTAsync_successData* response);
    
    static int onMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message);
    static void onConnectionLost(void* context, char* cause);

public slots:
    void publish(const QString &topic, const QString &payload);
    void subscribe(const QString &topic);

signals:
    void connected();
    void disconnected();
    void subscribed(const QString& topic);
    void messageReceived(const QString &topic, const QString &payload);
    void messagePublished(const QString &topic, const QString &payload);

private:
    MQTTAsync client;
};

#endif // MQTT_CLIENT_QT_H
