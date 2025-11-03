#include "constants.h"
#include "helpers.h"
#include <base64.hpp>

volatile bool recording = false;
volatile bool ledOn = false;

void interruptCallBack() { intFlag = 1; }

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);
    pinMode(BUTTON_DELETE, INPUT_PULLUP);
    pinMode(BUTTON_MOVE, INPUT_PULLUP);
    pinMode(BUTTON_ROTATE, INPUT_PULLUP);
    pinMode(BUTTON_SCREENSHOT, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    setupWifi();
    setupTime();
    mqttClient.initialize();
    mqttClient.connect();
    tone(BUZZER, NOTE_C5, NOTE_DURATION);
    mqttClient.subscribe(VOICE_RESULT); // subscribe to voice_result
    while (!SPIFFS.begin(true)) {
        mqttClient.publish(COMMAND, "ERROR INITIALIZING SPIFFS");
        delay(50);
    }

    Wire.setClock(100000);
    Wire.begin(); // start I2C comms

    mpu.initialize();
    tone(BUZZER, NOTE_C5, NOTE_DURATION);
    delay(50);
    Serial.println("MPU6050 connected. Calculating gyro bias...");
    calibrateGyroBias(mpu);
    tone(BUZZER, NOTE_C5, NOTE_DURATION);
    Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x,
                  gyro_bias_y, gyro_bias_z);

    delay(100);
    if (battMonitor.begin() == 0) {
        Serial.println("MAX17043 initialized.");
        tone(BUZZER, NOTE_C5, NOTE_DURATION);
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
    analogWrite(GREEN_PIN, 100);
    analogWrite(RED_PIN, 100);
    analogWrite(BLUE_PIN, 100);
    delay(50);
    analogWrite(GREEN_PIN, 0);
    analogWrite(RED_PIN, 0);
    analogWrite(BLUE_PIN, 0);

    //creates task to light up LED when recording
    xTaskCreatePinnedToCore(LedTask, "LedTask", 1536, NULL, 1, NULL, 1);
    i2sInit();
    Serial.println("I2S initialized");
}

void loop() {
    mqttClient.loop();
    // mpuLoop(mpu);
    unsigned long now = millis();
    static unsigned long button_debounce = 0;
    static unsigned long streaming_debounce = 0;
    static unsigned long batt_debounce = 0;
    if (now - button_debounce > DEBOUNCE) {
        bool sel = digitalRead(BUTTON_SELECT) == LOW;
        bool del = digitalRead(BUTTON_DELETE) == LOW;
        if (sel) {
            button_debounce = now;
            delay(20);
            recordVoice(0);
        } else if (del) {
            button_debounce = now;
            delay(20);
            recordVoice(1);
        } else if (digitalRead(BUTTON_SCREENSHOT) == LOW) {
            String message = "{\n\"type\": \"SCREENSHOT\"\n}";
            mqttClient.publish(COMMAND, message);
        }
        delay(20);
    }

    while (digitalRead(BUTTON_MOVE) == LOW || digitalRead(BUTTON_ROTATE) == LOW) {
        unsigned long now = millis();
        if (now - streaming_debounce > STREAMING_DEBOUNCE) {
            mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
            Serial.print(ax);
            Serial.print(" ");
            Serial.print(ay);
            Serial.print(" ");
            Serial.println(az);

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

    if ((now - batt_debounce) > DEBOUNCE * 6) {
        // only need to update battery percentage once in a while
        batt_debounce = now;
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
        Serial.printf("%2.4f, %2.2f\n", voltage, percentage);
        checkBattery(percentage);
    }

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