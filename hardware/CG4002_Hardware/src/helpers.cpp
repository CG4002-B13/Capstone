#include "helpers.h"

// ---- State ----
float pitch_deg = 0.0f, roll_deg = 0.0f, yaw_deg = 0.0f;
float gyro_bias_x = 0.0f, gyro_bias_y = 0.0f, gyro_bias_z = 0.0f;

float accx_lp=0, accy_lp=0, accz_lp=1;       // accel low-pass state
unsigned long last_ms;

// Gesture (twist around Z)
float yaw_delta_accum = 0.0f;                // integrates short-term yaw change
unsigned long last_gesture_ms = 0;

// ---- Helpers ----
bool isStill(float ax_g, float ay_g, float az_g, float gx_dps, float gy_dps, float gz_dps) {
  float gnorm = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
  float gyro_mag = sqrtf(gx_dps*gx_dps + gy_dps*gy_dps + gz_dps*gz_dps);
  return (fabsf(gnorm - 1.0f) < STILL_ACC_G_ERR) && (gyro_mag < STILL_GYRO_DPS);
}

void calibrateGyroBias(MPU6050 mpu) {
  long sx=0, sy=0, sz=0;
  for (int i=0; i<BIAS_CAL_SAMPLES; ++i) {
    int16_t ax,ay,az,gx,gy,gz;
    mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
    sx += gx; sy += gy; sz += gz;
    delay(2);
  }
  gyro_bias_x = (sx/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
  gyro_bias_y = (sy/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
  gyro_bias_z = (sz/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
}