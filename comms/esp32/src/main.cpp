#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "CertManager.h"

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;
char mqttServer[] = MQTT_SERVER;
const int mqttPort = MQTT_PORT;

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
CertificateManager certificateManager;

const char topic[] = "testing";

const long interval = 8000;
unsigned long previousMillis = 0;

void callback(char* topic, byte* payload, unsigned int length) {
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

void setupMQTT()
{
  Serial.begin(115200);

  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    // failed, retry
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  if (certificateManager.loadCertificates())
  {
    certificateManager.applyCertificates(wifiClient);
  }
  else
  {
    Serial.println("Using insecure connection");
    wifiClient.setInsecure();
  }

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void setup()
{
  // put your setup code here, to run once:
  setupMQTT();
}

void loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
    
    // Send test message every 30 seconds
    static unsigned long lastMsg = 0;
    unsigned long now = millis();
    if (now - lastMsg > 30000) {
        lastMsg = now;
        String msg = "Hello from ESP32: " + String(now);
        mqttClient.publish("esp32/data", msg.c_str());
    }
}
