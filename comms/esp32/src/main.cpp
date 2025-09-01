#include "CertificateManager.hpp"
#include "MQTTClient.hpp"
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASS;
const char mqttServer[] = MQTT_SERVER;
const char mqttPort[] = MQTT_PORT;

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);

const char topic[] = "testing";

const long interval = 8000;
unsigned long previousMillis = 0;

// For time syncing
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void setupMISC() {
    Serial.begin(115200);

    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        // failed, retry
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // Setup Time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Waiting for time sync...");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime synchronized");

    time_t now = time(nullptr);
    Serial.print("Current Time");
    Serial.println(ctime(&now));
}

void setup() {
    // put your setup code here, to run once:
    setupMISC();
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
