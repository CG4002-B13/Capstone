#include "constants.h"
#include "helpers.h"
#include <base64.hpp>

// WiFiClientSecure wifiClient;
// CertificateManager certificateManager;
// MQTTClient mqttClient(wifiClient, certificateManager);
// DFRobot_MAX17043 battMonitor;
// MPU6050 mpu;

int32_t data_buffer[512];
volatile bool recording = false;
volatile bool ledOn = false;
int16_t message[16003];

void interruptCallBack() { intFlag = 1; }

void setup() {
    Serial.begin(115200);
    setupWifi();
    setupTime();
    mqttClient.initialize();
    mqttClient.connect();
    mqttClient.subscribe(VOICE_RESULT); // subscribe to voice_result
    while (!SPIFFS.begin(true)) {
        mqttClient.publish(COMMAND, "ERROR INITIALIZING SPIFFS");
        delay(50);
    }

    Wire.setClock(100000);
    Wire.begin(); // start I2C comms

    mpu.initialize();
    delay(50);
    Serial.println("MPU6050 connected. Calculating gyro bias...");
    calibrateGyroBias(mpu);
    Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x,
                  gyro_bias_y, gyro_bias_z);

    delay(100);
    if (battMonitor.begin() == 0) {
        Serial.println("MAX17043 initialized.");
        //    battMonitor.setInterrupt(LOW_BATTERY); // sends an interrupt to
        //    warn when
        // the battery hits the threshold
    } else {
        Serial.println(
            "MAX17043 not initialized. Battery health not monitored.");
    }

    // set up RGB Led pins
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    //creates task to light up LED when recording
    xTaskCreatePinnedToCore(LedTask, "LedTask", 1024, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(batteryTask, "BatteryTask", 1024, NULL, 1, NULL, 0);
    i2sInit();
    Serial.println("I2S initialized");
    pinMode(BUTTON_SELECT, INPUT_PULLUP);     // select voice packet
    pinMode(BUTTON_DELETE, INPUT_PULLUP);     // delete voice packet
    pinMode(BUTTON_MOVE, INPUT_PULLUP);       // start axial movement
    pinMode(BUTTON_ROTATE, INPUT_PULLUP);     // start rotary movement
    pinMode(BUTTON_SCREENSHOT, INPUT_PULLUP); // start screenshot movement
    pinMode(BUZZER, OUTPUT);
}

void loop() {
    mqttClient.loop();
    // mpuLoop(mpu);
    unsigned long now = millis();
    static unsigned long button_debounce = 0;
    static unsigned long streaming_debounce = 0;
    static unsigned long batt_debounce = 0;
    int totalSamples = SAMPLING_RATE * RECORD_TIME;
    if (now - button_debounce > DEBOUNCE) {
        button_debounce = now;
        if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(BUTTON_DELETE) == LOW) {
            Serial.println("Voice Pressed");
            if (digitalRead(BUTTON_SELECT == LOW)) {
                message[0] = 0;
            } else if (digitalRead(BUTTON_DELETE == LOW)) {
                message[0] = 1;
            }
            size_t bytes_read = 0;
            int samplesWritten = 0;
            while (samplesWritten < totalSamples) {
                recording = true;
                i2s_read(I2S_NUM_0, &data_buffer, sizeof(data_buffer), &bytes_read, portMAX_DELAY);
                int samples = bytes_read / 4;
                for (int i = 0; i < samples; i++) {
                    int16_t temp = data_buffer[i] >> 14;
                    message[samplesWritten+i+1] = temp;
                    if (samplesWritten + i >= totalSamples) break;
                }
                samplesWritten += samples;
            }
            Serial.println("Recording done");
            recording = false;
            delay(20);
            for (int i = 0; i <= 2; i++) {
                int16_t *temp = &message[4000*i];
                message[4000*i] = message[0];
                if (mqttClient.publishBinary(VOICE_DATA, reinterpret_cast<uint8_t *>(temp), 8001 * sizeof(int16_t))) {
                    tone(BUZZER, NOTE_D5, NOTE_DURATION);
                } else {
                    tone(BUZZER, NOTE_D2, NOTE_DURATION);
                }
            }
        }
    }

    while (digitalRead(BUTTON_MOVE) == LOW || digitalRead(BUTTON_ROTATE) == LOW) {
        if (now - streaming_debounce > STREAMING_DEBOUNCE) {
            mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
            Serial.print(ax);
            Serial.print(" ");
            Serial.print(ay);
            Serial.print(" ");
            Serial.println(az);

            unsigned long now = millis();
            float dt = (now - last_ms) / 1000.0f;
            if (dt <= 0.0f)
                dt = 1e-3f;
            last_ms = now;

            // Convert to physical units
            AccX = ax / ACC_SCALE;
            AccY = ay / ACC_SCALE;
            AccZ = az / ACC_SCALE;

            streaming_debounce = now;
            String message = "{\n";
            if (digitalRead(BUTTON_MOVE) == LOW) {
                message += "\"type\": \"MOVE\"";
                //scaling for better movement
                AccX *=4;
                AccY *=4;
                AccZ *=4;
            } else if (digitalRead(BUTTON_ROTATE) == LOW) {
                message += "\"type\": \"ROTATE\"";
            }
            message += ",\n";
            String axes = "[";
            axes += String(AccX);
            axes += ", ";
            axes += String(AccY);
            axes += ", ";
            axes += String(AccZ);
            axes += "]";
            message += "\"axes\": ";
            message += axes;
            message += "\n}";
            mqttClient.publish(COMMAND, message);
        }
    }

    // if ((now - batt_debounce) > DEBOUNCE * 6) {
    //     // only need to update battery percentage once in a while
    //     batt_debounce = now;
    //     float voltage = battMonitor.readVoltage();
    //     float percentage = battMonitor.readPercentage();
    //     Serial.printf("%2.4f, %2.2f\n", voltage, percentage);
    //     checkBattery(percentage);
    // }

    if (intFlag == 1) {
        intFlag = 0;
        battMonitor.clearInterrupt();
        Serial.println("Low power alert interrupt!");
        tone(BUZZER, NOTE_A5, NOTE_DURATION);
        delay(NOTE_DURATION);
        noTone(BUZZER);
        delay(NOTE_DURATION);
        tone(BUZZER, NOTE_A5, NOTE_DURATION);
    }
}