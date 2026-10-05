/**
 * @file test_mic_filtered.cpp
 * @brief Heimdalls-Watch - INMP441 DSP Noise Cancellation & Threat Detection
 * @target ESP32-S3
 */

#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_SD_PIN   1   // Data Line
#define I2S_SCK_PIN  2   // Bit Clock (BCLK)
#define I2S_WS_PIN   42  // Word Select (LRCK)

#define I2S_PORT     I2S_NUM_0
#define SAMPLE_RATE  16000
#define BUFFER_LEN   512

int32_t raw_samples[BUFFER_LEN];

// DSP Filter Variables (IIR High-Pass Filter for DC Block)
float prev_raw = 0.0;
float prev_filtered = 0.0;
const float HPF_ALPHA = 0.985; // Cuts frequencies below ~80Hz (DC rumble block)

// Adaptive Noise Floor Variables
float ambientNoiseFloor = 200.0; // Running baseline ambient noise
const float NOISE_ADAPT_RATE = 0.02; // Slow adaptation to background noise

void initI2SMicrophone() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

/**
 * @brief 1st-Order IIR High-Pass Filter (DC Blocking)
 */
float applyHighPassFilter(float sample) {
    float filtered = HPF_ALPHA * (prev_filtered + sample - prev_raw);
    prev_raw = sample;
    prev_filtered = filtered;
    return filtered;
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);

    Serial.println("\n==========================================");
    Serial.println("[HEIMDALLS-WATCH] Filtered Mic DSP Initialized");
    Serial.println("==========================================");

    initI2SMicrophone();
}

void loop() {
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, &raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);

    if (result == ESP_OK && bytes_read > 0) {
        int samples_read = bytes_read / sizeof(int32_t);
        float min_val = 1e6;
        float max_val = -1e6;

        for (int i = 0; i < samples_read; i++) {
            // 1. Right-shift 24-bit audio inside 32-bit container
            float raw = (float)(raw_samples[i] >> 14);

            // 2. Apply High-Pass Filter to remove DC Offset and Low Rumble
            float cleanSample = applyHighPassFilter(raw);

            if (cleanSample > max_val) max_val = cleanSample;
            if (cleanSample < min_val) min_val = cleanSample;
        }

        // Calculate Clean Peak-to-Peak Amplitude
        float filteredPeakToPeak = max_val - min_val;

        // 3. Adaptive Noise Floor Calibration (Slowly tracks steady environment noise)
        if (filteredPeakToPeak < ambientNoiseFloor * 2.0) {
            ambientNoiseFloor = ((1.0 - NOISE_ADAPT_RATE) * ambientNoiseFloor) + (NOISE_ADAPT_RATE * filteredPeakToPeak);
        }

        // Calculate SNR-adjusted Threat Level above steady background
        float signalAboveNoise = filteredPeakToPeak - ambientNoiseFloor;
        if (signalAboveNoise < 0) signalAboveNoise = 0;

        // Format Timestamp [HH:MM:SS.mmm]
        unsigned long m = millis();
        unsigned long secs = m / 1000;
        unsigned long mins = (secs / 60) % 60;
        unsigned long hrs = (secs / 3600) % 24;

        // Print Diagnostic Payload
        Serial.printf("[%02lu:%02lu:%02lu] [DSP_MIC] RawP2P: %.0f | BaselineNoise: %.0f | CleanSignal: %.0f ", 
                      hrs, mins, secs % 60, filteredPeakToPeak, ambientNoiseFloor, signalAboveNoise);

        if (signalAboveNoise > 2500) {
            Serial.println("-> [THREAT ALERT: IMPACT / SCREAM]");
        } else if (signalAboveNoise > 400) {
            Serial.println("-> [SPEECH / LOCAL NOISE]");
        } else {
            Serial.println("-> [CLEAN / QUIET]");
        }
    }

    delay(100);
}