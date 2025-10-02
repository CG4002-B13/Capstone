#include "CertificateManager.hpp"
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

    pinMode(ALR_PIN, INPUT_PULLUP); // sets ALR_PIN to PULLUP mode
    attachInterrupt(digitalPinToInterrupt(ALR_PIN), interruptCallBack,
                    FALLING); // sets interrupt to ALR_PIN on FALLING edge

    Wire.setClock(100000);
    Wire.begin(); // start I2C comms

    mpu.initialize();
    Serial.println("MPU6050 connected. Calculating gyro bias...");
    calibrate_gyro_bias(mpu);
    Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x,
                  gyro_bias_y, gyro_bias_z);

    while (battMonitor.begin() != 0) {
        Serial.println("Couldn't connect to MAX17043. Retrying...");
        delay(2000);
    }
    delay(2);
    Serial.println("Battery monitor successfully connected!");
    battMonitor.setInterrupt(LOW_BATTERY); // sends an interrupt to warn when
                                           // the battery hits the threshold

    i2s_init();
    Serial.println("I2S initialized");
    pinMode(BUTTON_1, INPUT_PULLUP); // for the two switches
    // BUTTON_1 functions: select/deselect item
    pinMode(BUTTON_2, INPUT_PULLUP);
    // BUTTON_2 functions: toggle rotate/movement, when item is deselected send
    // voice message BUTTON_1 + BUTTON_2: Screenshot?
    pinMode(BUZZER, OUTPUT);
}

void loop() {

    //mqttClient.loop();
    mpu_loop(mpu);
    unsigned long now = millis();
    static unsigned long button_debounce = 0;
    if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
        // screenshot
    }
    switch (button_state) {
    case 0:
        // No item selected
        if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == HIGH) {
            tone(BUZZER, NOTE_D5, NOTE_DURATION); // select an item
            String message = "SELECT";
            //mqttClient.publish(topic2, message.c_str());
            // if acknowledge, send another buzz
            Serial.println(message);
            button_state = 1;
        } else if (digitalRead(BUTTON_2) == LOW && digitalRead(BUTTON_1 == HIGH)) {
            size_t total_samples = SAMPLING_RATE;
            size_t samples_recorded = 0;
            tone(BUZZER, NOTE_C5, NOTE_DURATION); // start recording
            while (samples_recorded < total_samples) {
                size_t bytes_read = 0;
                esp_err_t ret = i2s_read(I2S_NUM_0, data_buffer, sizeof(data_buffer), &bytes_read, 100/portTICK_PERIOD_MS);
                if (ret == ESP_OK && bytes_read > 0) {
                    size_t samples = bytes_read / 2; //16-bit samples
                    for (size_t i = 0; i < samples && samples_recorded < total_samples; i++, samples_recorded++) {
                        int16_t sample16 = data[i];
                        Serial.print(sample16);
                    }
                }
                yield();
            }
            delay(100); // to allow data to be stored properly
            //mqttClient.publish(topic0, "Incoming voice message");
            // char buffer[SAMPLING_RATE * sizeof(int16_t)];
            // memcpy(buffer, data, sizeof(buffer));
            // Serial.println(buffer);
            //mqttClient.publish(topic0, buffer);
            //mqttClient.publish(topic0, "Completed sending voice message");
            tone(BUZZER, NOTE_C6, NOTE_DURATION); // completed recording
        }
        break;
    case 1:
        // Item selected
        static unsigned long last_time = 0;
        if (now - last_time > DEBOUNCE) { // can deal w the debounce calibration later
            last_time = now;
            String message = "";
            message += "Pitch=%6.2f  Roll=%6.2f  Yaw=%6.2f\n", pitch_deg,
                roll_deg, yaw_deg;
            message += "AccX=%6.2f  AccY=%6.2f  AccZ=%6.2f", AccX, AccY, AccZ;
            // figure out how to move objects using this
            //mqttClient.publish(topic2, message.c_str());
            Serial.print("Sent message: " + message);
        }
        if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == HIGH) {
            // deselect item
            tone(BUZZER, NOTE_D5, NOTE_DURATION); // select an item
            String message = "DESELECT";
            Serial.println(message);
            //mqttClient.publish(topic2, message.c_str());
            // if acknowledge, send another buzz
            button_state = 0;
        } else if (digitalRead(BUTTON_2) == LOW &&
                   digitalRead(BUTTON_1) == HIGH) {
            String message = "TOGGLE"; //toggle between rotate and move
        }
        break;
    default:
        break;
    }
    // Output (rate-limit to keep serial readable)
    static unsigned long lastMillis = 0;
    if ((now - lastMillis) > DEBOUNCE) { // only need to update battery
                                              // percentage once in a while
        lastMillis = now;
        String message = "voltage: ";
        message += battMonitor.readVoltage();
        message += "mV\n";
        message += "percentage: ";
        message += battMonitor.readPercentage();
        message += "%\n";
        Serial.println(message);
        dacWrite(DAC_PIN, batt_rounding(battMonitor.readPercentage()));
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
