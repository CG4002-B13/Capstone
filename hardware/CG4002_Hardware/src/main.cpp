#include <Arduino.h>
#include "DFRobot_MAX17043.h"
#include "MPU6050.h"
#include "Wire.h"


//setup alert pin for Battery monitor
#ifdef __AVR__
  #define ALR_PIN 2
#else
  #define ALR_PIN D2
#endif

#define MPU_ADDR 0x70
#define MAX_ADDR 0x36 //hardcoding the I2C addresses for sensors
#define LOW_BATTERY 10
#define DEBOUNCE 2000 //constant debouncing of 2 seconds

DFRobot_MAX17043 battMonitor;
MPU6050 mpu;
int intFlag = 0; //interrupt flag

void interruptCallBack() {
  intFlag = 1;
}
// ---- Tuning ----
static const float GYRO_SCALE = 131.0f;      // LSB -> deg/s at ±250 dps
static const float ACC_SCALE  = 16384.0f;    // LSB -> g at ±2g
static const float COMP_ALPHA = 0.02f;       // accel weight in complementary filter (0.01..0.05)
static const float ACC_LP_TAU = 0.5f;        // seconds, accel low-pass time constant
static const float STILL_GYRO_DPS = 3.0f;    // deg/s threshold to consider "still"
static const float STILL_ACC_G_ERR = 0.06f;  // |norm(acc)-1g| threshold (in g)
static const int BIAS_CAL_SAMPLES = 800;

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

void calibrateGyroBias() {
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



void setup()
{
  Serial.begin(115200);
  delay(400);
  while(!Serial);

  pinMode(ALR_PIN, INPUT_PULLUP); //sets ALR_PIN to PULLUP mode
  attachInterrupt(digitalPinToInterrupt(ALR_PIN), interruptCallBack, FALLING); //sets interrupt to ALR_PIN on FALLING edge

  Wire.begin();

  mpu.initialize();
  Serial.println("MPU6050 connected. Calculating gyro bias...");
  calibrateGyroBias();
  Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x, gyro_bias_y, gyro_bias_z);

  while(battMonitor.begin() != 0) {
    Serial.println("Couldn't connect to MAX17043. Retrying...");
    delay(2000);
  }
  delay(2);
  Serial.println("Battery monitor successfully connected!");
  battMonitor.setInterrupt(LOW_BATTERY); //sends an interrupt to warn when the battery hits the threshold
}

void loop()
{
 // Read raw IMU
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Timing
  unsigned long now = millis();
  float dt = (now - last_ms) / 1000.0f;
  if (dt <= 0.0f) dt = 1e-3f;
  last_ms = now;

  // Convert to physical units
  float AccX = ax / ACC_SCALE;
  float AccY = ay / ACC_SCALE;
  float AccZ = az / ACC_SCALE;
  float GyroX = gx / GYRO_SCALE - gyro_bias_x;
  float GyroY = gy / GYRO_SCALE - gyro_bias_y;
  float GyroZ = gz / GYRO_SCALE - gyro_bias_z;

  // Low-pass accel (for more stable pitch/roll)
  float acc_alpha = dt / (ACC_LP_TAU + dt);     // 0..1
  accx_lp += acc_alpha * (AccX - accx_lp);
  accy_lp += acc_alpha * (AccY - accy_lp);
  accz_lp += acc_alpha * (AccZ - accz_lp);

  // Pitch/Roll from accel
  float accPitch_deg = atan2f(accy_lp, sqrtf(accx_lp*accx_lp + accz_lp*accz_lp)) * 180.0f / PI;
  float accRoll_deg  = atan2f(-accx_lp, accz_lp) * 180.0f / PI;

  // Integrate gyro angles
  float pitch_gyro = pitch_deg + GyroX * dt;
  float roll_gyro  = roll_deg  + GyroY * dt;
  float yaw_gyro   = yaw_deg   + GyroZ * dt;  // relative, will drift over minutes

  // Complementary fuse: accel corrects pitch/roll
  pitch_deg = (1.0f - COMP_ALPHA) * pitch_gyro + COMP_ALPHA * accPitch_deg;
  roll_deg  = (1.0f - COMP_ALPHA) * roll_gyro  + COMP_ALPHA * accRoll_deg;
  yaw_deg   = yaw_gyro;

  // Keep yaw in [-180, 180]
  if (yaw_deg > 180)  yaw_deg -= 360;
  if (yaw_deg < -180) yaw_deg += 360;

  // Adaptive gyro bias when still (slowly trims residual drift)
  if (isStill(AccX, AccY, AccZ, GyroX + gyro_bias_x, GyroY + gyro_bias_y, GyroZ + gyro_bias_z)) {
    float k = 0.002f; // small adaptation per cycle when still
    gyro_bias_x = (1.0f - k)*gyro_bias_x + k*(gx / GYRO_SCALE);
    gyro_bias_y = (1.0f - k)*gyro_bias_y + k*(gy / GYRO_SCALE);
    gyro_bias_z = (1.0f - k)*gyro_bias_z + k*(gz / GYRO_SCALE);
    // Optional gentle yaw damping while still (prevents slow creep)
    yaw_deg *= 0.999f;
  }

  // ---- Simple twist gesture (left/right) around Z ----
  // Integrate short-window yaw change and threshold it.
  yaw_delta_accum += GyroZ * dt;                // deg accumulated since last report
  if (fabsf(yaw_delta_accum) > 25.0f && (now - last_gesture_ms) > 250) {
    if (yaw_delta_accum > 0) Serial.println("[gesture] Twist Right");
    else                     Serial.println("[gesture] Twist Left");
    yaw_delta_accum = 0.0f;
    last_gesture_ms = now;
  }

  // Output (rate-limit to keep serial readable)
  static unsigned long last_print = 0;
  if (now - last_print > 2000) {
    Serial.printf("Pitch=%6.2f  Roll=%6.2f  Yaw=%6.2f\n", pitch_deg, roll_deg, yaw_deg);
    last_print = now;
  }

  static unsigned long lastMillis = 0;
  if((millis() - lastMillis) > DEBOUNCE) {
    lastMillis = millis();
    Serial.println();

    Serial.print("voltage: ");
    Serial.print(battMonitor.readVoltage());
    Serial.println(" mV");

    Serial.print("precentage: ");
    Serial.print(battMonitor.readPercentage());
    Serial.println(" %");
  }

  if(intFlag == 1) {
    intFlag = 0;
    battMonitor.clearInterrupt();
    Serial.println("Low power alert interrupt!");
    //put your battery low power alert interrupt service routine here
  }
}