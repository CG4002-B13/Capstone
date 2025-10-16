#include "Arduino.h"
#include "mbedtls/base64.h"
#include "FS.h"
#include "MPU6050.h"
#include "constants.h"
#include "driver/i2s.h"
#include "math.h"
#include "SPIFFS.h"
#include "Wire.h"
#include "ArduinoJson.h"
#include "DFRobot_MAX17043.h"
#include "CertificateManager.hpp"
#include "MQTTClient.hpp"
#include <PubSubClient.h>
#include <Setup.hpp>
#include <WiFiClientSecure.h>

extern float pitch_deg, roll_deg, yaw_deg;
extern float gyro_bias_x, gyro_bias_y, gyro_bias_z;

extern float accx_lp, accy_lp, accz_lp;
extern unsigned long last_ms;

extern float yaw_delta_accum;
extern unsigned long last_gesture_ms;

extern bool isRecording;

static const float GYRO_SCALE = 131.0f;  // LSB -> deg/s at ±250 dps
static const float ACC_SCALE = 16384.0f; // LSB -> g at ±2g
static const float COMP_ALPHA = 0.02f; // accel weight in complementary filter (0.01..0.05)
static const float ACC_LP_TAU = 0.5f; // seconds, accel low-pass time constant
static const float STILL_GYRO_DPS = 3.0f; // deg/s threshold to consider "still"
static const float STILL_ACC_G_ERR = 0.06f; // |norm(acc)-1g| threshold (in g)
static const int BIAS_CAL_SAMPLES = 800;

static int16_t ax, ay, az, gx, gy, gz;
static float AccX, AccY, AccZ, GyroX, GyroY, GyroZ;

static int intFlag = 0; //interrupt flag

bool isStill(float ax_g, float ay_g, float az_g, float gx_dps, float gy_dps,
              float gz_dps);
void calibrateGyroBias(MPU6050 mpu);
void mpuLoop(MPU6050 mpu);
void writeWavHeader(File &file);
void i2sInit();
void checkBattery(float percentage);
