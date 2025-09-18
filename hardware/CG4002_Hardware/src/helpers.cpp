#include "helpers.h"

<<<<<<< HEAD
float pitch_deg = 0.0f, roll_deg = 0.0f, yaw_deg = 0.0f;
float gyro_bias_x = 0.0f, gyro_bias_y = 0.0f, gyro_bias_z = 0.0f;

float accx_lp = 0, accy_lp = 0, accz_lp = 1;
unsigned long last_ms;

float yaw_delta_accum = 0.0f;
unsigned long last_gesture_ms = 0;

uint32_t total_samples = SAMPLING_RATE * RECORD_TIME;
uint32_t buffer_size = total_samples * (BITS_PER_SAMPLE / 8);
int16_t data[SAMPLING_RATE * RECORD_TIME];
bool isRecording = false;
const size_t chunk_size = 512;

// ---- Helpers ----
bool is_still(float ax_g, float ay_g, float az_g, float gx_dps, float gy_dps,
             float gz_dps) {
    float gnorm = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    float gyro_mag = sqrtf(gx_dps * gx_dps + gy_dps * gy_dps + gz_dps * gz_dps);
    return (fabsf(gnorm - 1.0f) < STILL_ACC_G_ERR) &&
           (gyro_mag < STILL_GYRO_DPS);
}

void calibrate_gyro_bias(MPU6050 mpu) {
    long sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < BIAS_CAL_SAMPLES; ++i) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        sx += gx;
        sy += gy;
        sz += gz;
        delay(2);
    }
    gyro_bias_x = (sx / (float)BIAS_CAL_SAMPLES) / GYRO_SCALE;
    gyro_bias_y = (sy / (float)BIAS_CAL_SAMPLES) / GYRO_SCALE;
    gyro_bias_z = (sz / (float)BIAS_CAL_SAMPLES) / GYRO_SCALE;
}

void mpu_loop(MPU6050 mpu) {
    // Read raw IMU
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

    // Low-pass accel (for more stable pitch/roll)
    float acc_alpha = dt / (ACC_LP_TAU + dt); // 0..1
    accx_lp += acc_alpha * (AccX - accx_lp);
    accy_lp += acc_alpha * (AccY - accy_lp);
    accz_lp += acc_alpha * (AccZ - accz_lp);

    // Pitch/Roll from accel
    float accPitch_deg =
        atan2f(accy_lp, sqrtf(accx_lp * accx_lp + accz_lp * accz_lp)) * 180.0f /
        PI;
    float accRoll_deg = atan2f(-accx_lp, accz_lp) * 180.0f / PI;

    // Integrate gyro angles
    float pitch_gyro = pitch_deg + GyroX * dt;
    float roll_gyro = roll_deg + GyroY * dt;
    float yaw_gyro = yaw_deg + GyroZ * dt; // relative, will drift over minutes

    // Complementary fuse: accel corrects pitch/roll
    pitch_deg = (1.0f - COMP_ALPHA) * pitch_gyro + COMP_ALPHA * accPitch_deg;
    roll_deg = (1.0f - COMP_ALPHA) * roll_gyro + COMP_ALPHA * accRoll_deg;
    yaw_deg = yaw_gyro;

    // Keep yaw in [-180, 180]
    if (yaw_deg > 180)
        yaw_deg -= 360;
    if (yaw_deg < -180)
        yaw_deg += 360;

    // Adaptive gyro bias when still (slowly trims residual drift)
    if (is_still(AccX, AccY, AccZ, GyroX + gyro_bias_x, GyroY + gyro_bias_y,
                 GyroZ + gyro_bias_z)) {
        float k = 0.002f; // small adaptation per cycle when still
        gyro_bias_x = (1.0f - k) * gyro_bias_x + k * (gx / GYRO_SCALE);
        gyro_bias_y = (1.0f - k) * gyro_bias_y + k * (gy / GYRO_SCALE);
        gyro_bias_z = (1.0f - k) * gyro_bias_z + k * (gz / GYRO_SCALE);
        // Optional gentle yaw damping while still (prevents slow creep)
        yaw_deg *= 0.999f;
    }
}

uint8_t batt_rounding(float percentage) {
    // convert eg 71.42 -> 70%
    int result = floor(percentage / 10); // floor for conservative estimate
    result = result * 255/10;
    Serial.print(result);
    return result;            // scaled to analog output
}

File writeWavHeader(uint32_t sampleRate, uint16_t bitsPerSample,
                    uint16_t channels, uint32_t dataSize) {
    File file;
    uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign = channels * bitsPerSample / 8;

    // RIFF header
    file.write((uint8_t *)"RIFF", 4);
    uint32_t chunkSize = 36 + dataSize;
    file.write((byte *)&chunkSize, 4);
    file.write((uint8_t *)"WAVE", 4);

    // fmt subchunk
    file.write((uint8_t *)"fmt ", 4);
    uint32_t subChunk1Size = 16;
    file.write((byte *)&subChunk1Size, 4);
    uint16_t audioFormat = 1; // PCM
    file.write((byte *)&audioFormat, 2);
    file.write((byte *)&channels, 2);
    file.write((byte *)&sampleRate, 4);
    file.write((byte *)&byteRate, 4);
    file.write((byte *)&blockAlign, 2);
    file.write((byte *)&bitsPerSample, 2);

    // data subchunk
    file.write((uint8_t *)"data", 4);
    file.write((byte *)&dataSize, 4);
    return file;
}

void i2s_init() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLING_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
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

void record_voice() {
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, (void *)data, sizeof(data), &bytes_read, portMAX_DELAY);
}

void publish_voice() {
    int16_t *ptr = &data[0]; // get the starting pointer
    for (size_t offset = 0; offset < total_samples; offset += chunk_size) {

    }
=======
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
>>>>>>> ef86164 (chore: Create helper files)
}