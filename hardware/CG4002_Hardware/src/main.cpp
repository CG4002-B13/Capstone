#include <Arduino.h>
#include "DFRobot_MAX17043.h"
#include "MPU6050.h"
#include "Wire.h"
#include "constants.h"
#include "helpers.h"


//setup alert pin for Battery monitor

DFRobot_MAX17043 battMonitor;
MPU6050 mpu;


int intFlag = 0; //interrupt flag
const char topic0[] = "esp32/voice_data";
const char topic1[] = "esp32/voice_result";
const char topic2[] = "esp32/gesture_data";

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




void setup()
{
  Serial.begin(115200);
  setupWifi();
  setupTime();
  mqttClient.initialize();
  mqttClient.connect();
  delay(400);
  while(!Serial);

  pinMode(ALR_PIN, INPUT_PULLUP); //sets ALR_PIN to PULLUP mode
  attachInterrupt(digitalPinToInterrupt(ALR_PIN), interruptCallBack, FALLING); //sets interrupt to ALR_PIN on FALLING edge

  Wire.begin();

  mpu.initialize();
  Serial.println("MPU6050 connected. Calculating gyro bias...");
  calibrateGyroBias(mpu);
  Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x, gyro_bias_y, gyro_bias_z);

  while(battMonitor.begin() != 0) {
    Serial.println("Couldn't connect to MAX17043. Retrying...");
    delay(2000);
  }
  delay(2);
  Serial.println("Battery monitor successfully connected!");
  battMonitor.setInterrupt(LOW_BATTERY); //sends an interrupt to warn when the battery hits the threshold

  //pinMode(D8, INPUT); //for the two switches
  //pinMode(D9, INPUT);

}

void loop()
{
  mqttClient.loop();
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
  static unsigned long lastMillis = 0;
  if((now - lastMillis) > DEBOUNCE) {
    lastMillis = now;
    String message = "voltage: ";
    message += battMonitor.readVoltage();
    message += "mV\n";
    message += "percentage: ";
    message += battMonitor.readPercentage();
    message += "%\n";
    message += "Pitch=%6.2f  Roll=%6.2f  Yaw=%6.2f\n", pitch_deg, roll_deg, yaw_deg;

    mqttClient.publish(topic2, message.c_str());
    Serial.print("Sent message: " + message);
    dacWrite(DAC_PIN, battRounding(battMonitor.readPercentage()));
  }

  if(intFlag == 1) {
    intFlag = 0;
    battMonitor.clearInterrupt();
    Serial.println("Low power alert interrupt!");
    tone(8, NOTE_A, NOTE_DURATION);
    tone(8, 0, NOTE_DURATION); //silence?
    tone(8, NOTE_A, NOTE_DURATION);
    tone(8, 0, NOTE_DURATION); //silence?
    tone(8, NOTE_A, NOTE_DURATION);
    //put your battery low power alert interrupt service routine here
  }
