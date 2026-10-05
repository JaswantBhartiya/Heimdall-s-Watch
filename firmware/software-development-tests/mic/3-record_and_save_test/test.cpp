/**
 * @file main.cpp
 * @brief Heimdalls-Watch - 5-Second WAV Audio Recorder to PC over Serial
 * @target ESP32-S3
 */

#include <Arduino.h>
#include <driver/i2s.h>

// I2S Hardware Pins
#define I2S_SD_PIN   1
#define I2S_SCK_PIN  2
#define I2S_WS_PIN   42
#define I2S_PORT     I2S_NUM_0

// Audio Specs
#define SAMPLE_RATE     16000
#define RECORD_TIME_SEC 5
#define TOTAL_SAMPLES   (SAMPLE_RATE * RECORD_TIME_SEC)
#define WAV_HEADER_SIZE 44

// Allocate buffer in internal SRAM (160,000 bytes for 5 seconds of 16-bit audio)
int16_t* audioBuffer = NULL;

void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
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

// Generate 44-byte Canonical WAV Header
void createWavHeader(uint8_t* header, uint32_t pcmDataSize) {
    uint32_t totalFileSize = pcmDataSize + 36;
    uint32_t byteRate = SAMPLE_RATE * 2; // 16-bit mono = 2 bytes per sample

    // RIFF Chunk
    memcpy(header, "RIFF", 4);
    memcpy(header + 4, &totalFileSize, 4);
    memcpy(header + 8, "WAVE", 4);

    // Format Chunk
    memcpy(header + 12, "fmt ", 4);
    uint32_t fmtChunkSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1; // Mono
    uint32_t sampleRate = SAMPLE_RATE;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;

    memcpy(header + 16, &fmtChunkSize, 4);
    memcpy(header + 20, &audioFormat, 2);
    memcpy(header + 22, &numChannels, 2);
    memcpy(header + 24, &sampleRate, 4);
    memcpy(header + 28, &byteRate, 4);
    memcpy(header + 32, &blockAlign, 2);
    memcpy(header + 34, &bitsPerSample, 2);

    // Data Chunk
    memcpy(header + 36, "data", 4);
    memcpy(header + 40, &pcmDataSize, 4);
}

void recordAndSendAudio() {
    Serial.println("\n[STATUS] Ready to record. 3...");
    delay(1000);
    Serial.println("[STATUS] 2...");
    delay(1000);
    Serial.println("[STATUS] 1...");
    delay(1000);
    Serial.println("[RECORDING_NOW]");

    size_t bytes_read = 0;
    int32_t rawSample[256];
    uint32_t samplesCaptured = 0;

    // High-pass filter variable
    float prev_raw = 0, prev_filt = 0;

    while (samplesCaptured < TOTAL_SAMPLES) {
        esp_err_t res = i2s_read(I2S_PORT, &rawSample, sizeof(rawSample), &bytes_read, portMAX_DELAY);
        if (res == ESP_OK && bytes_read > 0) {
            int count = bytes_read / sizeof(int32_t);
            for (int i = 0; i < count && samplesCaptured < TOTAL_SAMPLES; i++) {
                // Bit shift 24-bit audio inside 32-bit container
                float raw = (float)(rawSample[i] >> 14);

                // High-pass DC Blocking Filter
                float filtered = 0.985 * (prev_filt + raw - prev_raw);
                prev_raw = raw;
                prev_filt = filtered;

                // Scale & constrain to 16-bit signed integer range
                int16_t pcm16 = (int16_t)constrain(filtered, -32768, 32767);
                audioBuffer[samplesCaptured++] = pcm16;
            }
        }
    }

    Serial.println("[RECORDING_FINISHED]");
    delay(500);

    // Send Binary Sync Headers to PC
    Serial.println("<<<START_WAV_TRANSFER>>>");
    delay(100);

    // Send 44-Byte WAV Header
    uint8_t wavHeader[WAV_HEADER_SIZE];
    createWavHeader(wavHeader, TOTAL_SAMPLES * sizeof(int16_t));
    Serial.write(wavHeader, WAV_HEADER_SIZE);

    // Send Raw PCM Audio Data
    Serial.write((uint8_t*)audioBuffer, TOTAL_SAMPLES * sizeof(int16_t));
    Serial.flush();

    delay(100);
    Serial.println("\n<<<END_WAV_TRANSFER>>>");
}

void setup() {
    Serial.begin(921600); // High baud rate for fast file dumping
    while (!Serial && millis() < 3000);

    audioBuffer = (int16_t*)malloc(TOTAL_SAMPLES * sizeof(int16_t));
    if (!audioBuffer) {
        Serial.println("[ERROR] Failed to allocate memory buffer!");
        while (1);
    }

    initI2S();
}

void loop() {
    // Record every time user enters 'r' on serial or press enter
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == '\n') {
            recordAndSendAudio();
        }
    }
}
