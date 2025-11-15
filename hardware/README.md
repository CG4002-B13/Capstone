# Hardware

This is the hardware code for CG4002 B13. 

The main goal of this program is to run a loop, checking for one of 6 functions: _Select, Delete, Rotate, Move, Screenshot_ or )_Debug_. The full features are defined more clearly in the report.

## Getting Started
This project is run in PlatformIO. Running it in Arduino is probably possible, but is not tested.

As stated in the `platformio.ini` file, there are several libraries that are required to run this program:

| Library | Author | Version
|-|-| - |
| MAX1704X Fuel Gauge Arduino Library | Sparkfun | 1.0.4 |
| MPU6050 | Electronic Cats | 1.0.0 |
| PubSubClient | Knolleary | 2.8 |

To build this program, a `.env` file is needed. This is mainly for the MQTT Communications, and will be detailed more in that section. For security purposes, it has been added to `.gitignore`.

### Hardware Requirements

1. DFR0478 FireBeetle IoT ESP32
2. MPU6050
3. DFR0563 Gravity Fuel Gauge
4. INMP441 Omnidirectional Microphone
5. 1 RGB LED
6. 1 Active Buzzer
7. 5 Pushbuttons of any form
8. LiPo Battery

In this version, I used a 1200mAh battery, but smaller batteries can be used as well, depending on use duration and form factor.

The Schematics can be inferred using the pin definitions in `constants.h`. The full schematic drawing can be found in the report.
