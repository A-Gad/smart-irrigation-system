
#ifndef MQTT_HANDLER_HPP
#define MQTT_HANDLER_HPP

#include <string>
#include <functional>
#include <MQTTAsync.h>

class MqttHandler {
public:
    MqttHandler(const std::string& brokerAddress, const std::string& clientId);
    ~MqttHandler();

    void connect();
    void disconnect();
    void publish(const std::string& topic, const std::string& payload);
    void subscribe(const std::string& topic);

    // Callbacks to be implemented
    static void onConnect(void* context, MQTTAsync_successData* response);
    static void onConnectFailure(void* context, MQTTAsync_failureData* response);
    static int onMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message);
    static void onConnectionLost(void* context, char* cause);
    static void onReconnected(void* context, char* cause);
    

    using MessageCallback = std::function<void(std::string, std::string)>;
    void setCallback(MessageCallback callback);

private:
    std::string m_brokerAddress;
    std::string m_clientId;
    MQTTAsync m_client;
    MessageCallback m_callback;
};

#endif // MQTT_HANDLER_HPP
