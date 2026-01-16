
#include "mqtt_handler.hpp"
#include <cstring>
#include "logger.hpp"

MqttHandler::MqttHandler(const std::string& brokerAddress, const std::string& clientId) 
    : m_brokerAddress(brokerAddress), m_clientId(clientId) 
    {
    if(MQTTAsync_create(&m_client, brokerAddress.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL)
    != MQTTASYNC_SUCCESS)
    {
        spdlog::error("MQTT handler creation faild!");
    }
    spdlog::info("MQTT handler created successfully!");
}

MqttHandler::~MqttHandler() 
{
    MQTTAsync_destroy(&m_client);
}

void MqttHandler::connect() 
{
    MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.onSuccess = onConnect;
    opts.onFailure = onConnectFailure;
    opts.context = this;
    opts.automaticReconnect = 1;
    MQTTAsync_setCallbacks(m_client, this, onConnectionLost, onMessageArrived, nullptr);
    MQTTAsync_setConnected(m_client, this, onReconnected);
    MQTTAsync_connect(m_client, &opts);
}

void MqttHandler::disconnect() 
{
    MQTTAsync_disconnect(m_client, NULL);
    spdlog::info("MQTT disconnected successfully!");
}

void MqttHandler::publish(const std::string& topic, const std::string& payload) 
{
    //Create MQTTAsync_responseOptions
    MQTTAsync_responseOptions responseOpts = MQTTAsync_responseOptions_initializer;
    responseOpts.onSuccess = nullptr;
    responseOpts.onFailure = nullptr;
    responseOpts.context = nullptr;
    //Create MQTTAsync_message
    MQTTAsync_message message = MQTTAsync_message_initializer;
    message.payload = const_cast<char*>(payload.c_str());
    message.payloadlen = payload.size();
    message.qos = 1;
    message.retained = 0;
    //Call MQTTAsync_sendMessage
    MQTTAsync_sendMessage(m_client, topic.c_str(), &message, &responseOpts);
    spdlog::info("MQTT published requested!");
}

void MqttHandler::subscribe(const std::string& topic) 
{
    MQTTAsync_subscribe(m_client, topic.c_str(), 1, nullptr);
}

void MqttHandler::setCallback(MessageCallback callback) {
    m_callback = callback;
}

// Callbacks
void MqttHandler::onConnect(void* context, MQTTAsync_successData* response) {
    
    MqttHandler* handler = static_cast<MqttHandler*>(context);
    handler->subscribe("irrigation/command");
    spdlog::info("MQTT subscribed successfully to irrigation/command!");
}

void MqttHandler::onConnectFailure(void* context, MQTTAsync_failureData* response) 
{
    spdlog::error("connection failed");
}

int MqttHandler::onMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message) 
{
    // Handle incoming message payload
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    spdlog::info("MQTT message received: {}",payload);
    
    // Dispatch to registered callback
    MqttHandler* handler = static_cast<MqttHandler*>(context);
    if (handler->m_callback) {
        handler->m_callback(topicName, payload);
    }

    // Cleanup memory
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void MqttHandler::onConnectionLost(void* context, char* cause) 
{
    spdlog::warn("Connection Lost: {}", (cause ? cause : "Unknown"));
}

void MqttHandler::onReconnected(void* context, char* cause)
{
    MqttHandler* handler = static_cast<MqttHandler*>(context);
    handler->subscribe("irrigation/command");
    spdlog::info("MQTT reconnected and subscribed successfully!");
}
