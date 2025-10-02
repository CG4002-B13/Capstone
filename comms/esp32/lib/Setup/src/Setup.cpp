#include "Setup.hpp"
#include "time.h"
#include "esp_wpa2.h"
#include <WiFiClientSecure.h>

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void setupTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Waiting for time sync...");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime synchronized");

    time_t now = time(nullptr);
    Serial.print("Current Time: ");
    Serial.println(ctime(&now));
}

void setupWifi() {
    if (strcmp(ssid_mode, "PEAP") == 0) {
        Serial.print("Connecting to WPA2-Enterprise SSID: ");
        Serial.println(ssid);

        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);

        // Set WPA2 Enterprise credentials
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WIFI_SSID, strlen(WIFI_SSID));
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WIFI_USER, strlen(WIFI_USER));
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WIFI_PASS, strlen(WIFI_PASS));
        esp_wifi_sta_wpa2_ent_enable();
        WiFi.begin(ssid);
    } else {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        WiFi.begin(ssid, pass);
    }
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}
