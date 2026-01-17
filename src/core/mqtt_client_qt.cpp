#include "mqtt_client_qt.h"
#include <QDebug>
#include <cstring> // For strlen

MqttClientQt::MqttClientQt(QObject *parent) : QObject(parent), client(NULL) {
}

MqttClientQt::~MqttClientQt() {
    if (client) {
        MQTTAsync_destroy(&client);
    }
}

void MqttClientQt::connectToHost(const QString &host, int port) {
    if (client) {
        MQTTAsync_destroy(&client);
        client = NULL;
    }

    QString uri = QString("tcp://%1:%2").arg(host).arg(port);
    qDebug() << "Connecting to URI:" << uri;

    MQTTAsync_createOptions createOpts = MQTTAsync_createOptions_initializer;
    int rc = MQTTAsync_createWithOptions(&client, uri.toStdString().c_str(), 
                                         "QtClient", MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    
    if (rc != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to create client, return code:" << rc;
        return;
    }

    rc = MQTTAsync_setCallbacks(client, this, onConnectionLost, onMessageArrived, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to set callbacks, return code:" << rc;
    }

    MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
    connOpts.keepAliveInterval = 20;
    connOpts.cleansession = 1;
    connOpts.onSuccess = onConnectSuccess;
    connOpts.onFailure = onConnectFailure;
    connOpts.context = this;

    if ((rc = MQTTAsync_connect(client, &connOpts)) != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to start connect, return code:" << rc;
    }
}

void MqttClientQt::disconnectFromHost() {
    qDebug() << "Disconnecting...";
    if (!client) return;

    MQTTAsync_disconnectOptions discOpts = MQTTAsync_disconnectOptions_initializer;
    discOpts.onSuccess = onDisconnectSuccess;
    discOpts.context = this;

    int rc = MQTTAsync_disconnect(client, &discOpts);
    if (rc != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to start disconnect, return code:" << rc;
    }
}

void MqttClientQt::publish(const QString &topic, const QString &payload) {
    if (!client) return;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onPublishSuccess;
    opts.context = this;

    std::string payloadStr = payload.toStdString();
    int rc = MQTTAsync_send(client, topic.toStdString().c_str(), payloadStr.length(), payloadStr.c_str(), 1, 0, &opts);
    
    if (rc != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to publish message, return code:" << rc;
    }
    //messagePublished signal is emitted in the callback
}

void MqttClientQt::subscribe(const QString &topic) {
    if (!client) return;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onSubscribeSuccess;
    opts.context = this;

    int rc = MQTTAsync_subscribe(client, topic.toStdString().c_str(), 1, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        qDebug() << "Failed to subscribe, return code:" << rc;
    }
}

// Static Callbacks

void MqttClientQt::onConnectSuccess(void* context, MQTTAsync_successData* response) {
    Q_UNUSED(response);
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    emit self->connected();
}

void MqttClientQt::onConnectFailure(void* context, MQTTAsync_failureData* response) {
    Q_UNUSED(context);
    qDebug() << "Connect failed, rc:" << (response ? response->code : 0);
}

void MqttClientQt::onDisconnectSuccess(void* context, MQTTAsync_successData* response) {
    Q_UNUSED(response);
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    emit self->disconnected();
}

void MqttClientQt::onSubscribeSuccess(void* context, MQTTAsync_successData* response) {
    Q_UNUSED(response);
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    emit self->subscribed("topic_confirmed");
    qDebug() << "DEBUG: [MqttClientQt] Subscription successful!";
}

void MqttClientQt::onPublishSuccess(void* context, MQTTAsync_successData* response) {
    Q_UNUSED(response);
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    emit self->messagePublished("published_topic", "published_payload");
}

int MqttClientQt::onMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    
    QString topic;
    if (topicLen == 0) {
        topic = QString::fromUtf8(topicName);
    } else {
        topic = QString::fromUtf8(topicName, topicLen);
    }
    QString payload = QString::fromUtf8((char*)message->payload, message->payloadlen);
    
    qDebug() << "DEBUG: [MqttClientQt] Raw Paho Callback - Topic:" << topic << "Payload:" << payload;

    emit self->messageReceived(topic, payload);
    
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void MqttClientQt::onConnectionLost(void* context, char* cause) {
    MqttClientQt* self = static_cast<MqttClientQt*>(context);
    qDebug() << "Connection lost!" << (cause ? cause : "Unknown cause");
    emit self->disconnected();
}