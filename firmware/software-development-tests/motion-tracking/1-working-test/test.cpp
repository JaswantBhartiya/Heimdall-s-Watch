/**
 * @file main.cpp
 * @brief Heimdalls-Watch - MPU-6050 6-Axis Motion & Impact Diagnostic Test
 * @target ESP32-S3 (SDA: GPIO 8, SCL: GPIO 18)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// I2C Pin Definitions for ESP32-S3
#define I2C_SDA_PIN  8
#define I2C_SCL_PIN  18

Adafruit_MPU6050 mpu;

void scanI2CBus() {
    Serial.println("[I2C_SCAN] Scanning I2C bus...");
    byte count = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  -> Device found at address 0x%02X\n", address);
            count++;
        }
    }
    if (count == 0) {
        Serial.println("[ERROR] No I2C devices found! Check VCC, GND, SDA (GPIO 8), and SCL (GPIO 18).");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);

    Serial.println("\n[HEIMDALLS-WATCH] Testing I2C Bus...");

    // Initialize I2C with a 250ms timeout to prevent hanging
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setTimeOut(250); 

    scanI2CBus();

    
    // Initialize MPU-6050
    if (!mpu.begin(0x68, &Wire)) {
        Serial.println("[ERROR] Could not find MPU-6050 sensor chip! Retrying...");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("[SUCCESS] MPU-6050 initialized successfully.");

    // Configure sensor ranges for wearable motion/impact sensing
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);   // Up to 8G force detection
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);        // Fast rotation detection
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);      // Hardware noise filter
    
    Serial.println("[CONFIG] Accelerometer: ±8G | Gyro: ±500 deg/s | Filter: 21Hz\n");
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Calculate total G-force vector magnitude: G_total = sqrt(ax^2 + ay^2 + az^2) / 9.81
    float accelX_G = a.acceleration.x / 9.81;
    float accelY_G = a.acceleration.y / 9.81;
    float accelZ_G = a.acceleration.z / 9.81;
    
    float totalGForce = sqrt(accelX_G * accelX_G + 
                             accelY_G * accelY_G + 
                             accelZ_G * accelZ_G);

    // Print raw vector telemetry
    Serial.printf("[MOTION] Accel(G): X:%5.2f Y:%5.2f Z:%5.2f | Total: %4.2fG | Gyro(deg/s): Z:%6.1f ",
                  accelX_G, accelY_G, accelZ_G, totalGForce, g.gyro.z * (180.0 / PI));

    // Impact / Fall threshold evaluation
    if (totalGForce > 3.0) {
        Serial.println("-> [ALERT: HIGH-IMPACT COLLISION / FALL]");
    } else if (totalGForce > 1.8) {
        Serial.println("-> [WRIST MOVEMENT / SHAKE]");
    } else {
        Serial.println("-> [STABLE / RESTING]");
    }

    delay(200);
}
