#include "CertificateManager.hpp"
#include "MQTTClient.hpp"
#include "Setup.hpp"
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);

const char topic[] = "/gestures";

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

    // Sample send JSON (Option 1)
    StaticJsonDocument<BUFFER_SIZE> doc;
    doc["type"] = "MOVE";
    JsonArray axes = doc.createNestedArray("axes");
    axes.add(1.23); // x
    axes.add(4.56); // y
    axes.add(7.89); // z

    mqttClient.publishJson("esp32/gesture_data", doc, true);

    // Sample Send JSON (Option 2)
    const char* type = "MOVE";
    float axes[] = {1.23, 4.56, 7.89};
    int axesLength = sizeof(axes) / sizeof(axes[0]);

    String payload = "{\"type\":\"" + String(type) + "\",\"axes\":[";

    // Append each element manually
    for (int i = 0; i < axesLength; i++) {
        payload += String((float)axes[i], 2); // 2 decimal places
        if (i < axesLength - 1) payload += ","; // comma between elements
    }

    payload += "]}";

    mqttClient.publish("esp32/gesture_data", payload.c_str());
}
