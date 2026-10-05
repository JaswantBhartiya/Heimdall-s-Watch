/**
 * @file test_mic.cpp
 * @brief Heimdalls-Watch - INMP441 Microphone Test & Acoustic Level Monitor
 * @target ESP32-S3
 */

#include <Arduino.h>
#include <driver/i2s.h>

// Pins assigned for INMP441 on ESP32-S3
#define I2S_SD_PIN   1   // Data Line
#define I2S_SCK_PIN  2   // Bit Clock (BCLK)
#define I2S_WS_PIN   42  // Word Select / Left-Right Clock (LRCK)

#define I2S_PORT     I2S_NUM_0
#define SAMPLE_RATE  16000
#define BUFFER_LEN   512

int32_t samples[BUFFER_LEN];

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
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
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

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait for serial connection

    Serial.println("\n==========================================");
    Serial.println("[HEIMDALLS-WATCH] INMP441 Mic Diagnostic Test");
    Serial.println("==========================================");

    initI2SMicrophone();
    Serial.println("[MIC_TEST] I2S Driver Started. Monitoring audio streams...\n");
}

void loop() {
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);

    if (result == ESP_OK && bytes_read > 0) {
        int samples_read = bytes_read / sizeof(int32_t);
        int32_t min_sample = 2147483647;
        int32_t max_sample = -2147483648;

        for (int i = 0; i < samples_read; i++) {
            // Right-shift 24-bit data from 32-bit frame
            int32_t val = samples[i] >> 14; 
            if (val > max_sample) max_sample = val;
            if (val < min_sample) min_sample = val;
        }

        int32_t peakToPeak = max_sample - min_sample;

        // Uptime timestamp calculation [HH:MM:SS.mmm]
        unsigned long m = millis();
        unsigned long secs = m / 1000;
        unsigned long mins = (secs / 60) % 60;
        unsigned long hrs = (secs / 3600) % 24;
        unsigned long millisecs = m % 1000;

        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "[%02lu:%02lu:%02lu.%03lu]", hrs, mins, secs % 60, millisecs);

        // Visual sound intensity bar
        int barLength = map(constrain(peakToPeak, 0, 8000), 0, 8000, 0, 20);
        char bar[21];
        for (int b = 0; b < 20; b++) {
            bar[b] = (b < barLength) ? '#' : '.';
        }
        bar[20] = '\0';

        Serial.printf("%s [MIC] Peak-to-Peak: %6d | Level: [%s] ", timeStr, peakToPeak, bar);

        if (peakToPeak > 3500) {
            Serial.println("-> [IMPACT / SCREAM DETECTED]");
        } else if (peakToPeak > 600) {
            Serial.println("-> [VOICE / AMBIENT NOISE]");
        } else {
            Serial.println("-> [QUIET]");
        }
    }

    delay(100);
}