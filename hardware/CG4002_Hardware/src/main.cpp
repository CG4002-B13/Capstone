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
static unsigned long batt_debounce = 0;
static bool BLOCK = false;

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
    pinMode(BUTTON_SELECT, INPUT_PULLUP); // select voice packet
    pinMode(BUTTON_DELETE, INPUT_PULLUP); // delete voice packet
    pinMode(BUTTON_MOVE, INPUT_PULLUP); // start axial movement
    pinMode(BUTTON_ROTATE, INPUT_PULLUP); // start rotary movement
    pinMode(BUTTON_SCREENSHOT, INPUT_PULLUP); // start screenshot movement
    //Button 1 + 4 == Screenshot
    pinMode(BUZZER, OUTPUT);
}

void loop() {
    mqttClient.loop();
    mpuLoop(mpu);
    JsonDocument doc;
    static unsigned long now = millis();
    if (now - button_debounce > DEBOUNCE && !BLOCK) {
        button_debounce = now;
        size_t bytes_read = 0;
        if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(BUTTON_DELETE) == LOW) {
            Serial.println("Voice Pressed");
            BLOCK = true;

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
            analogWrite(BLUE_PIN, 255);
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
                    analogWrite(BLUE_PIN, 200);
                    analogWrite(RED_PIN, 200);
                } else {
                    tone(BUZZER, NOTE_D2, NOTE_DURATION);
                    analogWrite(GREEN_PIN, 200);
                    analogWrite(BLUE_PIN, 200);
                }
            }
        }
        
        if (digitalRead(BUTTON_MOVE) == LOW) {
            if (now - streaming_debounce > STREAMING_DEBOUNCE) {
                streaming_debounce = now;
                JsonDocument doc;
                doc["type"] = "MOVE";

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
            }
        } else if (digitalRead(BUTTON_ROTATE) == LOW) {
            if (now - streaming_debounce > STREAMING_DEBOUNCE) {
                streaming_debounce = now;
                JsonDocument doc;
                doc["type"] = "ROTATE";

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
            }
        }
    }

    if ((now - batt_debounce) > DEBOUNCE * 20) { // only need to update battery
                                         // percentage once in a while
        batt_debounce = now;
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
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