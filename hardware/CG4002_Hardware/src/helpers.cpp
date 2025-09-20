#include "helpers.h"

// ---- State ----
float pitch_deg = 0.0f, roll_deg = 0.0f, yaw_deg = 0.0f;
float gyro_bias_x = 0.0f, gyro_bias_y = 0.0f, gyro_bias_z = 0.0f;

float accx_lp=0, accy_lp=0, accz_lp=1;       // accel low-pass state
unsigned long last_ms;

// Gesture (twist around Z)
float yaw_delta_accum = 0.0f;                // integrates short-term yaw change
unsigned long last_gesture_ms = 0;

// ---- Helpers ----
bool isStill(float ax_g, float ay_g, float az_g, float gx_dps, float gy_dps, float gz_dps) {
  float gnorm = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
  float gyro_mag = sqrtf(gx_dps*gx_dps + gy_dps*gy_dps + gz_dps*gz_dps);
  return (fabsf(gnorm - 1.0f) < STILL_ACC_G_ERR) && (gyro_mag < STILL_GYRO_DPS);
}

void calibrateGyroBias(MPU6050 mpu) {
  long sx=0, sy=0, sz=0;
  for (int i=0; i<BIAS_CAL_SAMPLES; ++i) {
    int16_t ax,ay,az,gx,gy,gz;
    mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
    sx += gx; sy += gy; sz += gz;
    delay(2);
  }
  gyro_bias_x = (sx/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
  gyro_bias_y = (sy/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
  gyro_bias_z = (sz/(float)BIAS_CAL_SAMPLES)/GYRO_SCALE;
}

uint8_t battRounding(float percentage) {
  //convert eg 71.42 -> 70% 
  int result = round(percentage / 10);
  return result * 255 / 10; //scaled to analog output
}

void i2sInit() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 gives 24-bit data
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1, // not used
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);
} 

File writeWavHeader(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t dataSize) {
    File file;
    uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign = channels * bitsPerSample / 8;

    // RIFF header
    file.write("RIFF", 4);
    uint32_t chunkSize = 36 + dataSize;
    file.write((byte*)&chunkSize, 4);
    file.write("WAVE", 4);

    // fmt subchunk
    file.write("fmt ", 4);
    uint32_t subChunk1Size = 16;
    file.write((byte*)&subChunk1Size, 4);
    uint16_t audioFormat = 1; // PCM
    file.write((byte*)&audioFormat, 2);
    file.write((byte*)&channels, 2);
    file.write((byte*)&sampleRate, 4);
    file.write((byte*)&byteRate, 4);
    file.write((byte*)&blockAlign, 2);
    file.write((byte*)&bitsPerSample, 2);

    // data subchunk
    file.write("data", 4);
    file.write((byte*)&dataSize, 4);
    return file;
}