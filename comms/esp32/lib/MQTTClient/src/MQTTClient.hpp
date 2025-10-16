#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "CertificateManager.hpp"
#include <ArduinoJson.h>
#include <map>
#include <functional>

const char MQTT_HOST[] = MQTT_SERVER;
const char MQTT_PORT_NUMBER[] = MQTT_PORT;
const char MQTT_USERNAME[] = MQTT_USER;
const char MQTT_PASSWORD[] = MQTT_PASS;
const char MQTT_STATUS_TOPIC[] = "esp32/status";
const char MQTT_COMMAND_TOPIC[] = "esp32/command";

const unsigned long MQTT_RECONNECT_DELAY = 5000;
const unsigned long MESSAGE_PUBLISH_INTERVAL = 3000;
const uint16_t BUFFER_SIZE = 8192;

class MQTTClient {
private:
    WiFiClientSecure& wifiClient;
    CertificateManager& certificateManager;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;
    unsigned long lastMessageTime;
    static std::map<String, std::function<void(const String&)>> topicCallbacks;
    
    String generateClientId();
    bool loadCertificates();

public:
    MQTTClient(WiFiClientSecure& client, CertificateManager& certManager);
    
    bool initialize();
    bool connect();
    bool isConnected();
    void handleReconnection();
    void loop();
    void publishStatus(const String& status);
    bool publish(const String& topic, const String& message, bool retain = false);
    bool subscribe(const String& topic);
    bool publishJson(const String& topic, JsonDocument& doc, bool retain);
    
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    void registerCallback(const String& topic, std::function<void(const String& message)> callback);
    
    static MQTTClient* instance;
};

#endif