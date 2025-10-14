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
    // ledcSetup(0, 5000, 8);
    // ledcAttachPin(GREEN_PIN, 0);
    // ledcSetup(1, 5000, 8);
    // ledcAttachPin(RED_PIN, 1);
    // ledcSetup(2, 5000, 8);
    // ledcAttachPin(BLUE_PIN, 2);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    //set up RGB Led pins

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
    mqttClient.loop();
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    unsigned long now = millis();
    float dt = (now - last_ms) / 1000.0f;
    if (dt <= 0.0f)
        dt = 1e-3f;
    last_ms = now;

    // Convert to physical units
    AccX = ax / ACC_SCALE;
    AccY = ay / ACC_SCALE;
    AccZ = az / ACC_SCALE;
    GyroX = gx / GYRO_SCALE - gyro_bias_x;
    GyroY = gy / GYRO_SCALE - gyro_bias_y;
    GyroZ = gz / GYRO_SCALE - gyro_bias_z;

    static unsigned long button_debounce = 0;
    if (now - button_debounce > DEBOUNCE / 5) {
        button_debounce = now;
        size_t bytes_read = 0;
        if (digitalRead(BUTTON_2) == LOW) {
            Serial.println("Button2 Pressed");
            File file = SPIFFS.open("output.wav", FILE_WRITE);
            int tries = 0;
            while (tries < 5) {
                if (!file) {
                    Serial.println("Failed to open file. Retrying...");
                    tries++;
                    file = SPIFFS.open("output.wav", FILE_WRITE);
                } else {
                    tries = 5;
                }
            }
            delay(500);
            int totalSamples = SAMPLING_RATE * RECORD_TIME;
            writeWavHeader(file);
            Serial.print("Starting audio recording...");
            delay(500);
            int samplesWritten = 0;
            while (samplesWritten < SAMPLING_RATE * RECORD_TIME) {
                i2s_read(I2S_NUM_0, &data_buffer, sizeof(data_buffer),
                                        &bytes_read, portMAX_DELAY);
                int samples = bytes_read / 2;
                for (int i = 0; i < samples; i++) {
                    file.write((uint8_t *)&data_buffer[i], 2);
                    samplesWritten++;
                    if (samplesWritten >= totalSamples) break;
                }
            }
            file.close();
            uint8_t buf[512];
            File f = SPIFFS.open("output.wav", FILE_READ);
            if (!f) {
                tries = 0;
                while (tries < 5) {
                    mqttClient.publish(topic0, "Failed to open WAV file. Retrying...\n");
                    tries++;
                    f = SPIFFS.open("output.wav", FILE_READ);
                }
            }
            while (f.available()) {
                String message = "";
                size_t n = f.read(buf, sizeof(buf));
                message += (char * )buf;
                if (mqttClient.publish(topic0, message)) {
                    tone(BUZZER, NOTE_D5, NOTE_DURATION);
                } else {
                    tone(BUZZER, NOTE_D2, NOTE_DURATION);
                }
            }
        } else if (digitalRead(BUTTON_1) == LOW) {
            Serial.print("Button1 Pressed");
            if (object_state == "move") {
                object_state = "rotate";
            } else {
                object_state = "move";
            }
        }

        //check for voice_data


        String packet = "";
        if ((object_state != "deselect") && object_state.length() > 0) {
            packet = "type : ";
            packet += object_state;
            char sub_packet[60];
            sprintf(sub_packet, "%6.2f, %6.2f, %6.2f", AccX, AccY, AccZ);
            packet += ", \n";
            packet += sub_packet;
            packet += "\n";
        }

        if (object_state != "" && object_state != "deselect") {
            mqttClient.publish(topic2, packet);
        } 
    }

    static unsigned long lastMillis = 0;
    if ((now - lastMillis) > DEBOUNCE) { // only need to update battery
                                         // percentage once in a while
        lastMillis = now;
        String batt_message = "voltage: ";
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
        batt_message += voltage;
        batt_message += "mV\n";
        batt_message += "percentage: ";
        batt_message += percentage;
        batt_message += "%\n";
        Serial.println(batt_message);
        check_battery(percentage);
        // figure out how to move objects using this
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
