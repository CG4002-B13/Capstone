#ifndef SETUP_HPP
#define SETUP_HPP

const char ssid_mode[] = WIFI_MODE;
const char ssid[] = WIFI_SSID;
const char user[] = WIFI_USER;
const char pass[] = WIFI_PASS;

extern const char *ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

void setupTime();
void setupWifi();

#endif
