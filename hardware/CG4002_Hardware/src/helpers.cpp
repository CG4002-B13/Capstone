#include "helpers.h"

WiFiClientSecure wifiClient;
CertificateManager certificateManager;
MQTTClient mqttClient(wifiClient, certificateManager);
DFRobot_MAX17043 battMonitor;
MPU6050 mpu;

float pitch_deg = 0.0f, roll_deg = 0.0f, yaw_deg = 0.0f;
float gyro_bias_x = 0.0f, gyro_bias_y = 0.0f, gyro_bias_z = 0.0f;

float accx_lp = 0, accy_lp = 0, accz_lp = 1;
unsigned long last_ms;

float yaw_delta_accum = 0.0f;
unsigned long last_gesture_ms = 0;

// ---- Helpers ----

void checkBattery(float perc) {
    if (perc > 70.0) {
        analogWrite(GREEN_PIN, 100);
        delay(50);
        analogWrite(GREEN_PIN, 255);
    } else if (perc > 40.0) {
        analogWrite(RED_PIN, 100);
        analogWrite(GREEN_PIN, 255);
        delay(50);
        analogWrite(RED_PIN, 255);
        analogWrite(GREEN_PIN, 255);
    } else {
        analogWrite(RED_PIN, 100);
        delay(50);
        analogWrite(RED_PIN, 255);
    }
}

void LedTask(void *parameter) {
    while (1) {
        if (recording && !ledOn) {
            analogWrite(BLUE_PIN, 100);
            ledOn = true;
        } else if (!recording) {
            analogWrite(BLUE_PIN, 255);
            ledOn = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void i2sInit() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLING_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

    const i2s_pin_config_t pin_config = {.bck_io_num = MIC_SCK,
                                         .ws_io_num = MIC_WS,
                                         .data_out_num = I2S_PIN_NO_CHANGE,
                                         .data_in_num = MIC_SD};

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void recordVoice(int16_t flag) {
    message[0] = flag;
    int32_t data_buffer[512];
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
    Serial.println(message[0]);
    recording = false;
    delay(20);
    sendVoice();
}

void sendVoice() {
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