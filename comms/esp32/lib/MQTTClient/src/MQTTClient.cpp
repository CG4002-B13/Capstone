#include "MQTTClient.hpp"
#include "esp_system.h"
#include <Arduino.h>

MQTTClient* MQTTClient::instance = nullptr;
std::map<String, std::function<void(const String&)>> MQTTClient::topicCallbacks;

MQTTClient::MQTTClient(WiFiClientSecure& client, CertificateManager& certManager)
    : wifiClient(client), certificateManager(certManager), 
      mqttClient(client), lastReconnectAttempt(0), lastMessageTime(0) {
    instance = this;
}

String MQTTClient::generateClientId() {
    return "ESP32Client-" + String(random(0xffff), HEX);
}

bool MQTTClient::loadCertificates() {
    Serial.println("Loading SSL certificates...");
    
    if (!certificateManager.loadCertificates()) {
        Serial.println("ERROR: Failed to load certificates");
        return false;
    }
    
    certificateManager.applyCertificates(&wifiClient);
    return true;
}

bool MQTTClient::initialize() {
    if (!loadCertificates()) {
        Serial.println("WARNING: SSL certificate loading failed. Connection may be insecure.");
    }
    
    mqttClient.setServer(MQTT_HOST, atoi(MQTT_PORT_NUMBER));
    mqttClient.setCallback(messageCallback);
    mqttClient.setBufferSize(BUFFER_SIZE);
    
    randomSeed(esp_random());
    
    Serial.println("MQTT client initialized.");
    return true;
}

bool MQTTClient::connect() {
    if (mqttClient.connected()) return true;
    
    Serial.print("Connecting to MQTT broker...");
    String clientId = generateClientId();
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
        Serial.println(" connected!");
        
        publishStatus("online");
        
        if (mqttClient.subscribe(MQTT_COMMAND_TOPIC)) {
            Serial.println("Subscribed to command topic.");
        } else {
            Serial.println("WARNING: Failed to subscribe to command topic.");
        }
        
        return true;
    }
    
    Serial.print(" failed! Error code: ");
    Serial.println(mqttClient.state());
    return false;
}

bool MQTTClient::isConnected() {
    return mqttClient.connected();
}

void MQTTClient::handleReconnection() {
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY) {
        lastReconnectAttempt = now;
        
        Serial.println("Attempting MQTT reconnection...");
        if (!connect()) {
            Serial.print("MQTT reconnection failed. Retrying in ");
            Serial.print(MQTT_RECONNECT_DELAY / 1000);
            Serial.println(" seconds...");
        }
    }
}

void MQTTClient::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    } else {
        handleReconnection();
    }
}

void MQTTClient::publishStatus(const String& status) {
    if (mqttClient.connected()) {
        mqttClient.publish(MQTT_STATUS_TOPIC, status.c_str(), true); // retained
        Serial.println("Status published: " + status);
    }
}

bool MQTTClient::publish(const String& topic, const String& message, bool retain) {
    if (mqttClient.connected()) {
        bool result = mqttClient.publish(topic.c_str(), message.c_str(), retain);
        if (result) {
            Serial.println("Published to " + topic + ": " + message);
        } else {
            Serial.println("ERROR: Failed to publish to " + topic);
        }
        return result;
    }
    Serial.println("ERROR: Cannot publish - MQTT not connected!");
    return false;
}

bool MQTTClient::subscribe(const String& topic) {
    if (mqttClient.connected()) {
        bool result = mqttClient.subscribe(topic.c_str());
        if (result) {
            Serial.println("Subscribed to: " + topic);
        } else {
            Serial.println("ERROR: Failed to subscribe to: " + topic);
        }
        return result;
    }
    Serial.println("ERROR: Cannot subscribe - MQTT not connected!");
    return false;
}

bool MQTTClient::publishJson(const String& topic, JsonDocument& doc, bool retain = false) {
    char buffer[BUFFER_SIZE];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    String jsonMessage(buffer, len);
    
    return publish(topic, jsonMessage, retain);
}

void MQTTClient::registerCallback(const String& topic, std::function<void(const String& message)> callback) {
    topicCallbacks[topic] = callback;
}

void MQTTClient::messageCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    String topicStr = String(topic);
    Serial.printf("Message received [%s]: %s\n", topic, message.c_str());

    auto it = topicCallbacks.find(topicStr);
    if (it != topicCallbacks.end()) {
        it->second(message); 
    } else {
        Serial.println("No callback registered for this topic.");
    }
}