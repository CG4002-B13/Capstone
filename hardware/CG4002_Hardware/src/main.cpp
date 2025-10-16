#include "constants.h"
#include "helpers.h"

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);
DFRobot_MAX17043 battMonitor;
MPU6050 mpu;

static int16_t data_buffer[SAMPLING_RATE];
static String object_state = "";

static unsigned long button_debounce = 0;
static unsigned long streaming_debounce = 0;

void interruptCallBack() { intFlag = 1; }

void setup() {
    Serial.begin(115200);
    setupWifi();
    setupTime();
    mqttClient.initialize();
    mqttClient.connect();
    mqttClient.subscribe(VOICE_RESULT); //subscribe to voice_result
    while (!SPIFFS.begin(true)) {
        mqttClient.publish(COMMAND, "ERROR INITIALIZING SPIFFS");
        delay(50);
    }

    Wire.setClock(100000);
    Wire.begin(); // start I2C comms

    mpu.initialize();
    mqttClient.publish(COMMAND, "MPU6050 connected. Calculating gyro bias...");
    calibrateGyroBias(mpu);
    Serial.printf("Gyro bias (dps): x=%.3f y=%.3f z=%.3f\n", gyro_bias_x,
                  gyro_bias_y, gyro_bias_z);

    while (battMonitor.begin() != 0) {
        Serial.println("Couldn't connect to MAX17043. Retrying...");
        delay(200);
    }
    delay(2);
    Serial.println("Battery monitor successfully connected!");
    battMonitor.setInterrupt(LOW_BATTERY); // sends an interrupt to warn when
                                           // the battery hits the threshold
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    //set up RGB Led pins

    i2sInit();
    Serial.println("I2S initialized");
    pinMode(BUTTON_1, INPUT_PULLUP); // select voice packet
    pinMode(BUTTON_2, INPUT_PULLUP); // delete voice packet
    pinMode(BUTTON_3, INPUT_PULLUP); // start axial movement
    pinMode(BUTTON_4, INPUT_PULLUP); // start rotary movement
    //Button 1 + 4 == Screenshot
    pinMode(BUZZER, OUTPUT);
}

void loop() {
    mqttClient.loop();
    mpuLoop(mpu);
    JsonDocument doc;
    static unsigned long now = millis();
    if (now - button_debounce > DEBOUNCE) {
        button_debounce = now;
        size_t bytes_read = 0;
        if (digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
            Serial.println("Voice Pressed");

            if (digitalRead(BUTTON_1 == LOW)) {
                doc["TYPE"] = "SELECT";
            } else if (digitalRead(BUTTON_2 == LOW)) {
                doc["TYPE"] = "DELETE";
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
                    mqttClient.publish(VOICE_DATA, "Failed to open WAV file. Retrying...\n");
                    tries++;
                    f = SPIFFS.open("output.wav", FILE_READ);
                }
            }
            while (f.available()) {
                size_t dataLen = sizeof(buf);
                unsigned char base64Voice[dataLen*2];
                size_t bytesWritten = 0;
                size_t encodedLen = mbedtls_base64_encode(base64Voice, sizeof(base64Voice), &bytesWritten, buf, sizeof(buf));
                size_t n = f.read(buf, sizeof(buf));
                String message = reinterpret_cast<char*>(base64Voice);
                if (mqttClient.publish(VOICE_DATA, message)) {
                    tone(BUZZER, NOTE_D5, NOTE_DURATION);
                } else {
                    tone(BUZZER, NOTE_D2, NOTE_DURATION);
                }
            }
        }
        
        if (digitalRead(BUTTON_3) == LOW || digitalRead(BUTTON_4) == LOW) {
            JsonDocument doc;
            if (digitalRead(BUTTON_3) == LOW) {
                doc["type"] = "MOVE";
            } else if (digitalRead(BUTTON_4) == LOW) {
                doc["type"] = "ROTATE";
            }
            JsonArray data = doc["axes"].to<JsonArray>();
            data.add(AccX);
            data.add(AccY);
            data.add(AccZ);
            }

            mqttClient.publishJson(COMMAND, doc, true);
        }

    }

    if ((now - streaming_debounce) > STREAMING_DEBOUNCE) { // only need to update battery
                                         // percentage once in a while
        streaming_debounce = now;
        String batt_message = "voltage: ";
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
        batt_message += voltage;
        batt_message += "mV\n";
        batt_message += "percentage: ";
        batt_message += percentage;
        batt_message += "%\n";
        Serial.println(batt_message);
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