#include "CertificateManager.hpp"
#include "MQTTClient.hpp"
#include "Setup.hpp"
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);

const char topic[] = "testing";

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    setupWifi();
    setupTime();
    mqttClient.initialize();
    mqttClient.connect();
}

void loop() {
    mqttClient.loop();

    // Send test message every 30 seconds
    static unsigned long lastMsg = 0;
    unsigned long now = millis();
    if ((now - lastMsg > 3000) && mqttClient.isConnected()) {
        lastMsg = now;
        String msg = "Hello from ESP32: " + String(now);
        mqttClient.publish(topic, msg.c_str());
        Serial.print("Sent message: ");
        Serial.println(msg.c_str());
    }
}
