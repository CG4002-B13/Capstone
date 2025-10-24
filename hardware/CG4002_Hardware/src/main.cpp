#include "constants.h"
#include "helpers.h"

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);
DFRobot_MAX17043 battMonitor;
MPU6050 mpu;

static int16_t data_buffer[SAMPLING_RATE];

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
    if (now - button_debounce > DEBOUNCE) {
        button_debounce = now;
        size_t bytes_read = 0;
        if (digitalRead(BUTTON_SELECT) == LOW ^
            digitalRead(BUTTON_DELETE) == LOW) {
            Serial.println("Voice Pressed");
            /*
            if (digitalRead(BUTTON_SELECT == LOW)) {
                doc["type"] = "SELECT";
            } else if (digitalRead(BUTTON_DELETE == LOW)) {
                doc["type"] = "DELETE";
            }
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
            if (file) {
                analogWrite(BLUE_PIN, 150);
                delay(250);
                analogWrite(BLUE_PIN, 0);
            }
            writeWavHeader(file);
            delay(250);
            int samplesWritten = 0;
            while (samplesWritten < SAMPLING_RATE * RECORD_TIME) {
                analogWrite(BLUE_PIN, 150);
                i2s_read(I2S_NUM_0, &data_buffer, sizeof(data_buffer),
                                        &bytes_read, portMAX_DELAY);
                int samples = bytes_read / 4;
                for (int i = 0; i < samples; i++) {
                    int16_t sample = data_buffer[i] >> 14;
                    file.write((uint8_t *)&data_buffer[i], 2);
                    samplesWritten++;
                    if (samplesWritten >= totalSamples) break;
                }
            }
            analogWrite(BLUE_PIN, 150);
            file.close();
            uint8_t buf[512];
            File f = SPIFFS.open("output.wav", FILE_READ);
            if (!f) {
                tries = 0;
                while (tries < 5) {
                    mqttClient.publish(VOICE_DATA, "Failed to open WAV file.
            Retrying...\n"); tries++; f = SPIFFS.open("output.wav", FILE_READ);
                }
            }
            while (f.available()) {
                size_t dataLen = sizeof(buf);
                unsigned char base64Voice[dataLen*2];
                size_t bytesWritten = 0;
                size_t encodedLen = mbedtls_base64_encode(base64Voice,
            sizeof(base64Voice), &bytesWritten, buf, sizeof(buf)); size_t n =
            f.read(buf, sizeof(buf)); String message =
            reinterpret_cast<char*>(base64Voice); if
            (mqttClient.publish(VOICE_DATA, message)) { tone(BUZZER, NOTE_D5,
            NOTE_DURATION); analogWrite(BLUE_PIN, 200); analogWrite(RED_PIN,
            200); } else { tone(BUZZER, NOTE_D2, NOTE_DURATION);
                    analogWrite(GREEN_PIN, 200);
                    analogWrite(BLUE_PIN, 200);
                }
            }*/
        }
    }

    if (digitalRead(BUTTON_MOVE) == LOW || digitalRead(BUTTON_ROTATE) == LOW) {
        if (now - streaming_debounce > STREAMING_DEBOUNCE) {
            mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
            Serial.print(ax);
            Serial.print(" ");
            Serial.print(ay);
            Serial.print(" ");
            Serial.print(az);
            Serial.print(" ");
            Serial.print(gx);
            Serial.print(" ");
            Serial.print(gy);
            Serial.print(" ");
            Serial.println(gz);

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

            // Low-pass accel (for more stable pitch/roll)
            float acc_alpha = dt / (ACC_LP_TAU + dt); // 0..1
            accx_lp += acc_alpha * (AccX - accx_lp);
            accy_lp += acc_alpha * (AccY - accy_lp);
            accz_lp += acc_alpha * (AccZ - accz_lp);

            // Pitch/Roll from accel
            float accPitch_deg =
                atan2f(accy_lp, sqrtf(accx_lp * accx_lp + accz_lp * accz_lp)) *
                180.0f / PI;
            float accRoll_deg = atan2f(-accx_lp, accz_lp) * 180.0f / PI;

            // Integrate gyro angles
            float pitch_gyro = pitch_deg + GyroX * dt;
            float roll_gyro = roll_deg + GyroY * dt;
            float yaw_gyro =
                yaw_deg + GyroZ * dt; // relative, will drift over minutes

            // Complementary fuse: accel corrects pitch/roll
            pitch_deg =
                (1.0f - COMP_ALPHA) * pitch_gyro + COMP_ALPHA * accPitch_deg;
            roll_deg = (1.0f - COMP_ALPHA) * roll_gyro + COMP_ALPHA * accRoll_deg;
            yaw_deg = yaw_gyro;

            // Keep yaw in [-180, 180]
            if (yaw_deg > 180)
                yaw_deg -= 360;
            if (yaw_deg < -180)
                yaw_deg += 360;

            // Adaptive gyro bias when still (slowly trims residual drift)
            if (isStill(AccX, AccY, AccZ, GyroX + gyro_bias_x, GyroY + gyro_bias_y,
                        GyroZ + gyro_bias_z)) {
                float k = 0.002f; // small adaptation per cycle when still
                gyro_bias_x = (1.0f - k) * gyro_bias_x + k * (gx / GYRO_SCALE);
                gyro_bias_y = (1.0f - k) * gyro_bias_y + k * (gy / GYRO_SCALE);
                gyro_bias_z = (1.0f - k) * gyro_bias_z + k * (gz / GYRO_SCALE);
                // Optional gentle yaw damping while still (prevents slow creep)
                yaw_deg *= 0.999f;
            }
            streaming_debounce = now;
            String message = "{\n";
            if (digitalRead(BUTTON_MOVE) == LOW) {
                message += "\"type\": \"MOVE\"";
            } else if (digitalRead(BUTTON_ROTATE) == LOW) {
                message += "\"type\": \"ROTATE\"";
            }
            message += ",\n";
            if (AccX > AccY && AccX > AccZ) {
                AccY = 0.0;
                AccZ = 0.0;
            } else if (AccY > AccX && AccY > AccZ) {
                AccX = 0.0;
                AccZ = 0.0;
            } else if (AccZ > AccX && AccZ > AccY) {
                AccX = 0.0;
                AccY = 0.0;
            }
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
            /*
            JsonDocument doc;
            if (digitalRead(BUTTON_MOVE) == LOW) {
                doc["type"] = "MOVE";
            } else if (digitalRead(BUTTON_ROTATE) == LOW) {
                doc["type"] = "ROTATE";
            }

            JsonArray data = doc["axes"].to<JsonArray>();
            if (AccX > AccY && AccX > AccZ) {
                AccY = 0.0;
                AccZ = 0.0;
            } else if (AccY > AccX && AccY > AccZ) {
                AccX = 0.0;
                AccZ = 0.0;
            } else if (AccZ > AccX && AccZ > AccY) {
                AccX = 0.0;
                AccY = 0.0;
            }
            data.add(AccX);
            data.add(AccY);
            data.add(AccZ);
            mqttClient.publishJson(COMMAND, doc, true);
            */
        }
    }

    if ((now - batt_debounce) > DEBOUNCE * 6) {
        // only need to update battery percentage once in a while
        batt_debounce = now;
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
        Serial.printf("%2.4f, %2.2f\n", voltage, percentage);
        checkBattery(percentage);
        // figure out how to move objects using this
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