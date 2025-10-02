#ifndef SETUP_HPP
#define SETUP_HPP

extern const char ssid_mode[] = WIFI_MODE;
extern const char ssid[] = WIFI_SSID;
extern const char user[] = WIFI_USER;
extern const char pass[] = WIFI_PASS;

extern const char *ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

void setupTime();
void setupWifi();

#endif
