#include "constants.h"
#include "helpers.h"

volatile bool recording = false;
volatile bool ledOn = false;
volatile bool first = true;
struct timeval tv;

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
    //Serial.println("MPU6050 connected. Calculating gyro bias...");
    tone(BUZZER, NOTE_C5, NOTE_DURATION);

    delay(100);
    if (battMonitor.begin() == 0) {
        //Serial.println("MAX17043 initialized.");
        tone(BUZZER, NOTE_C5, NOTE_DURATION);
        //    battMonitor.setInterrupt(LOW_BATTERY); // sends an interrupt to
        //    warn when
        // the battery hits the threshold
    } else {
        //Serial.println("MAX17043 not initialized. Battery health not monitored.");
    }

    // set up RGB Led pins
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    analogWrite(GREEN_PIN, 100);
    analogWrite(RED_PIN, 100);
    analogWrite(BLUE_PIN, 100);
    delay(50);
    analogWrite(GREEN_PIN, 255);
    analogWrite(RED_PIN, 255);
    analogWrite(BLUE_PIN, 255);

    //creates task to light up LED when recording
    xTaskCreatePinnedToCore(LedTask, "LedTask", 1536, NULL, 1, NULL, 1);
    i2sInit();
    //Serial.println("I2S initialized");
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
            button_debounce = now;
            String message = "{\n\"type\": \"SCREENSHOT\"\n}";
            mqttClient.publish(COMMAND, message);
        }
        delay(250); //blocks other commands between 
    }

    while (digitalRead(BUTTON_MOVE) == LOW || digitalRead(BUTTON_ROTATE) == LOW) {
        unsigned long now = millis();
        if (now - streaming_debounce > STREAMING_DEBOUNCE) {
            static int16_t x1,y1,z1, ax, ay, az;
            mpu.getAcceleration(&ax, &ay, &az);
            float dx = (float) ax;
            float dy = (float) ay;
            float dz = (float) az;
            dx = (0.01 * dx) + (0.09 * dx) + (0.9 * dx);
            dy = (0.01 * dy) + (0.09 * dy) + (0.9 * dy);
            dz = (0.01 * dz) + (0.09 * dz) + (0.9 * dz);
            if (first) {
                x1 = dx;
                y1 = dy;
                z1 = dz;
                first = false;
            }

            //ignore small values
            if (dx < 0.2) {
                dx = 0;
            }
            if (dy < 0.2) {
                dy = 0;
            }
            if (dz < 0.2) {
                dz = 0;
            }

            // Convert to physical units
            AccX = -2 * (dx - x1) / ACC_SCALE;
            AccY = -2 * (dz - z1) / ACC_SCALE;
            AccZ = -2 * (dy - y1) / ACC_SCALE;

            // Lock to maximum
            if (AccX > 2) {
                AccX = 2;
            }
            if (AccY > 2) {
                AccY = 2;
            }
            if (AccZ > 2) {
                AccZ = 2;
            }
            if (abs(AccY) > abs(AccX) && abs(AccY) > abs(AccZ)) {
                AccX = 0;
                AccZ = 0;
            } else if (abs(AccX) > abs(AccY) && abs(AccX) > abs(AccZ)) {
                AccY = 0;
                AccZ = 0;
            } else if (abs(AccZ) > abs(AccY) && abs(AccZ) > abs(AccX)) {
                AccY = 0;
                AccX = 0;
            }

            streaming_debounce = now;
            String message = "{\n";
            if (digitalRead(BUTTON_MOVE) == LOW && digitalRead(BUTTON_ROTATE) == LOW) { //debug
                String message = "{\n\"type\": \"DEBUG\",\n";
                gettimeofday(&tv, NULL);
                unsigned long long time_now = (unsigned long long) (tv.tv_sec) * 1000 + (unsigned long long) (tv.tv_usec) / 1000;
                message += "\"timestamp\": ";
                message += time_now;
                message += "\n}";
                mqttClient.publish(COMMAND, message);
                streaming_debounce += 2000; //prevent spamming of debug
                sendVoice();
            } else if (digitalRead(BUTTON_MOVE) == LOW && digitalRead(BUTTON_ROTATE) == HIGH) {
                message += "\"type\": \"MOVE\"";
                //scaling for better movement
                AccX *= 2;
                AccY *= 2.2;
                AccZ *= 2.5;
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
            } else if (digitalRead(BUTTON_ROTATE) == LOW && digitalRead(BUTTON_MOVE) == HIGH) {
                message += "\"type\": \"ROTATE\"";
                message += ",\n";
                if (AccZ > 0) AccZ *= 1.5;
                String axes = "[";
                axes += String(AccY);
                axes += ", ";
                axes += String(AccZ);
                axes += ", ";
                axes += String(AccX);
                axes += "]";
                message += "\"axes\": ";
                message += axes;
                message += "\n}";
                mqttClient.publish(COMMAND, message);
            }
        }
    }
    first = true;

    if ((now - batt_debounce) > BATTERY_DEBOUNCE) {
        // only need to update battery percentage once in a while
        batt_debounce = now;
        float voltage = battMonitor.readVoltage();
        float percentage = battMonitor.readPercentage();
        //Serial.printf("%2.4f, %2.2f\n", voltage, percentage);
        checkBattery(percentage);
    }

    if (intFlag == 1) {
        intFlag = 0;
        battMonitor.clearInterrupt();
        //Serial.println("Low power alert interrupt!");
        tone(BUZZER, NOTE_A5, NOTE_DURATION);
        delay(NOTE_DURATION);
        noTone(BUZZER);
        delay(NOTE_DURATION);
        tone(BUZZER, NOTE_A5, NOTE_DURATION);
    }
}