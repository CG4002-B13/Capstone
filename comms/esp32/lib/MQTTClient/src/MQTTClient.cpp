#include "MQTTClient.hpp"
#include <Arduino.h>

MQTTClient* MQTTClient::instance = nullptr;

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
    Serial.println("SSL certificates applied successfully.");
    return true;
}

bool MQTTClient::initialize() {
    if (!loadCertificates()) {
        Serial.println("WARNING: SSL certificate loading failed. Connection may be insecure.");
    }
    
    // mqttClient.setServer(MQTT_HOST, atoi(MQTT_PORT_NUMBER));
    mqttClient.setServer("192.168.1.3", 8883);
    mqttClient.setCallback(messageCallback);
    
    // randomSeed(analogRead(0));
    
    Serial.println("MQTT client initialized.");
    return true;
}

bool MQTTClient::connect() {
    if (mqttClient.connected()) return true;
    
    Serial.print("Connecting to MQTT broker...");
    String clientId = generateClientId();
    
    if (mqttClient.connect(clientId.c_str())) {
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

void MQTTClient::messageCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("Message received [%s]: ", topic);
    
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.println(message);
    
    // Handle commands
    if (String(topic) == MQTT_COMMAND_TOPIC) {
        handleCommand(message);
    }
}

void MQTTClient::handleCommand(const String& command) {
    Serial.println("Processing command: " + command);
    
    if (command == "restart") {
        Serial.println("Restart command received. Rebooting...");
        if (instance) {
            instance->publishStatus("restarting");
        }
        delay(1000);
        ESP.restart();
    } else if (command == "status") {
        if (instance) {
            instance->publishStatus("Device running normally");
        }
    } else if (command == "uptime") {
        if (instance) {
            String uptimeMsg = "Uptime: " + String(millis() / 1000) + " seconds";
            instance->publish("esp32/uptime", uptimeMsg);
        }
    } else {
        Serial.println("Unknown command: " + command);
        if (instance) {
            instance->publish("esp32/error", "Unknown command: " + command);
        }
    }
}