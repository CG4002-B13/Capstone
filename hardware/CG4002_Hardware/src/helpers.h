#include "Arduino.h"
#include "math.h"
#include "MPU6050.h"
#include <I2S.h>

static const float GYRO_SCALE = 131.0f;      // LSB -> deg/s at ±250 dps
static const float ACC_SCALE  = 16384.0f;    // LSB -> g at ±2g
static const float COMP_ALPHA = 0.02f;       // accel weight in complementary filter (0.01..0.05)
static const float ACC_LP_TAU = 0.5f;        // seconds, accel low-pass time constant
static const float STILL_GYRO_DPS = 3.0f;    // deg/s threshold to consider "still"
static const float STILL_ACC_G_ERR = 0.06f;  // |norm(acc)-1g| threshold (in g)
static const int BIAS_CAL_SAMPLES = 800;

bool isStill(float ax_g, float ay_g, float az_g, float gx_dps, float gy_dps, float gz_dps);
void calibrateGyroBias(MPU6050 mpu);
uint8_t battRounding(float percentage);
File writeWavHeader(int sample_rate, int bits_per_sample, int channels, int data_size);
void i2sinit();
