/**
 * @file main.cpp
 * @brief Heimdalls-Watch - Circular Ring Buffer Audio Recorder (10s Rolling Loop)
 * @target ESP32-S3
 */

#include <Arduino.h>
#include <driver/i2s.h>

// Hardware Pins
#define I2S_SD_PIN   1
#define I2S_SCK_PIN  2
#define I2S_WS_PIN   42
#define I2S_PORT     I2S_NUM_0

// Audio Configuration
#define SAMPLE_RATE       16000
#define BUFFER_TIME_SEC   10   // Rolling memory window in seconds
#define TOTAL_SAMPLES     (SAMPLE_RATE * BUFFER_TIME_SEC)
#define WAV_HEADER_SIZE   44

// Ring Buffer in SRAM/PSRAM
int16_t* ringBuffer = NULL;
size_t writeIndex = 0;
bool isBufferFull = false;
bool isRecordingPaused = false;

// Filter variables
float prev_raw = 0, prev_filt = 0;

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

void createWavHeader(uint8_t* header, uint32_t pcmDataSize) {
    uint32_t totalFileSize = pcmDataSize + 36;
    uint32_t byteRate = SAMPLE_RATE * 2;

    memcpy(header, "RIFF", 4);
    memcpy(header + 4, &totalFileSize, 4);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    
    uint32_t fmtChunkSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
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
    memcpy(header + 36, "data", 4);
    memcpy(header + 40, &pcmDataSize, 4);
}

void streamBufferToPC() {
    isRecordingPaused = true; // Freeze buffer during export

    Serial.println("\n[EVENT] SOS/SAVE Triggered! Exporting rolling audio...");
    delay(100);

    Serial.println("<<<START_WAV_TRANSFER>>>");
    delay(100);

    // Send WAV Header
    uint8_t wavHeader[WAV_HEADER_SIZE];
    uint32_t actualSamples = isBufferFull ? TOTAL_SAMPLES : writeIndex;
    createWavHeader(wavHeader, actualSamples * sizeof(int16_t));
    Serial.write(wavHeader, WAV_HEADER_SIZE);

    // Stream samples in chronological order (Oldest -> Newest)
    if (isBufferFull) {
        // Part 1: Oldest samples (from current write position to end of array)
        Serial.write((uint8_t*)(ringBuffer + writeIndex), (TOTAL_SAMPLES - writeIndex) * sizeof(int16_t));
        // Part 2: Newest samples (from start of array up to current write position)
        Serial.write((uint8_t*)ringBuffer, writeIndex * sizeof(int16_t));
    } else {
        Serial.write((uint8_t*)ringBuffer, writeIndex * sizeof(int16_t));
    }

    Serial.flush();
    delay(100);
    Serial.println("\n<<<END_WAV_TRANSFER>>>");

    // Clear buffer state and resume continuous loop
    writeIndex = 0;
    isBufferFull = false;
    isRecordingPaused = false;
    Serial.println("[STATUS] Buffer reset. Continuous audio loop resumed.");
}

void setup() {
    Serial.begin(921600);
    while (!Serial && millis() < 3000);

    // Allocate ring buffer (Uses PSRAM if available, or SRAM)
    size_t bufferSizeBytes = TOTAL_SAMPLES * sizeof(int16_t);
    if (psramFound()) {
        ringBuffer = (int16_t*)ps_malloc(bufferSizeBytes);
        Serial.println("[MEMORY] Allocated 10-second ring buffer in PSRAM.");
    } else {
        ringBuffer = (int16_t*)malloc(bufferSizeBytes);
        Serial.println("[MEMORY] Allocated 10-second ring buffer in SRAM.");
    }

    if (!ringBuffer) {
        Serial.println("[ERROR] Failed to allocate memory!");
        while (1);
    }

    initI2S();
    Serial.println("[SYSTEM] Continuous rolling audio buffer active. Enter 's' to dump capture to PC.");
}

void loop() {
    // 1. Process User Commands
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 's' || cmd == 'S') {
            streamBufferToPC();
        }
    }

    // 2. Continuous Audio Capture Loop
    if (!isRecordingPaused) {
        size_t bytes_read = 0;
        int32_t rawSample[128];
        
        esp_err_t res = i2s_read(I2S_PORT, &rawSample, sizeof(rawSample), &bytes_read, 0);
        if (res == ESP_OK && bytes_read > 0) {
            int count = bytes_read / sizeof(int32_t);
            for (int i = 0; i < count; i++) {
                float raw = (float)(rawSample[i] >> 14);

                // High-Pass Filter (DC Block)
                float filtered = 0.985 * (prev_filt + raw - prev_raw);
                prev_raw = raw;
                prev_filt = filtered;

                int16_t pcm16 = (int16_t)constrain(filtered, -32768, 32767);

                // Circular Overwrite
                ringBuffer[writeIndex] = pcm16;
                writeIndex++;

                if (writeIndex >= TOTAL_SAMPLES) {
                    writeIndex = 0;
                    isBufferFull = true; // Ring buffer wrapped around
                }
            }
        }
    }
}
