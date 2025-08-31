#include "CertManager.hpp"
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;
char mqttServer[] = MQTT_SERVER;
char mqttPort[] = MQTT_PORT;

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
CertificateManager certificateManager;

const char topic[] = "testing";

const long interval = 8000;
unsigned long previousMillis = 0;

// For time syncing
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.printf("Message arrived [%s]: ", topic);
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void reconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");

        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("connected");
            mqttClient.publish("esp32/status", "online");
            mqttClient.subscribe("esp32/command");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setupMQTT() {
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

    if (certificateManager.loadCertificates()) {
        certificateManager.applyCertificates(wifiClient);
    } else {
        Serial.println("Unable to load certificates.");
    }

    // Setup MQTT
    // mqttClient.setServer(mqttServer, atoi(mqttPort));
    mqttClient.setServer("192.168.1.3", 8883);
    mqttClient.setCallback(callback);
}

void setup() {
    // put your setup code here, to run once:
    setupMQTT();
}

void loop() {
    if (!mqttClient.connected()) {
        Serial.println("Failed to connect to MQTT, trying again...");
        reconnect();
    }
    mqttClient.loop();

    // Send test message every 30 seconds
    static unsigned long lastMsg = 0;
    unsigned long now = millis();
    if (now - lastMsg > 3000) {
        lastMsg = now;
        String msg = "Hello from ESP32: " + String(now);
        mqttClient.publish(topic, msg.c_str());
        Serial.print("Sent message: ");
        Serial.println(msg.c_str());
    }
}
