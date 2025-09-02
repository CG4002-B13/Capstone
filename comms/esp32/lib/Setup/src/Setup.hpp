#ifndef SETUP_HPP
#define SETUP_HPP

extern const char ssid[];
extern const char pass[];

extern const char *ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

void setupTime();
void setupWifi();

#endif
