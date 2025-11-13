#include "Arduino.h"
#include "FS.h"
#include "MPU6050.h"
#include "constants.h"
#include "driver/i2s.h"
#include "math.h"
#include "SPIFFS.h"
#include "Wire.h"
#include "DFRobot_MAX17043.h"
#include "CertificateManager.hpp"
#include "MQTTClient.hpp"
#include <PubSubClient.h>
#include <Setup.hpp>
#include <WiFiClientSecure.h>

extern WiFiClientSecure wifiClient;
extern CertificateManager certificateManager;
extern MQTTClient mqttClient;
extern DFRobot_MAX17043 battMonitor;
extern MPU6050 mpu;

extern float accx_lp, accy_lp, accz_lp;
extern unsigned long last_ms;


extern volatile bool recording;
extern volatile bool ledOn;

static const float ACC_SCALE = 16384.0f; // LSB -> g at ±2g
static const float COMP_ALPHA = 0.02f; // accel weight in complementary filter (0.01..0.05)
static const float ACC_LP_TAU = 0.5f; // seconds, accel low-pass time constant

static int16_t ax, ay, az, gx, gy, gz;
static float AccX, AccY, AccZ, GyroX, GyroY, GyroZ;

static int intFlag = 0; //interrupt flag
static int totalSamples = SAMPLING_RATE * RECORD_TIME;
static int16_t message[16003];

void LedTask(void *parameter);
void checkBattery(float perc);
void i2sInit();
void recordVoice(int16_t flag);
void sendVoice();
