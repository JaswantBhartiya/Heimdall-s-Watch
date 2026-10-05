/**
 * @file test.cpp (rf_sniffer.cpp)
 * @brief Heimdalls-Watch - Part 1: Passive RF Surrounding Presence Sniffer
 * @details Captures nearby smartphone Wi-Fi Probe Requests (MACs + RSSI) 
 *          and BLE Advertisements (iPhones, Androids, Smartwatches).
 * @target ESP32-S3 Microcontroller
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ==========================================
// CONFIGURATION & STRUCTURES
// ==========================================
#define MAX_LOGGED_DEVICES 50
#define BLE_SCAN_TIME_SECONDS 3 // Duration of BLE scan cycle

// IEEE 802.11 Frame Header Structure for Wi-Fi Sniffing
typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t  addr1[6]; // Destination MAC
    uint8_t  addr2[6]; // Transmitter / Source MAC (Target Phone)
    uint8_t  addr3[6]; // BSSID
    uint16_t seq_ctrl;
} wifi_ieee80211_mac_header_t;

// Recorded Device Entry
struct ScannedDevice {
    char macStr[18];
    int rssi;
    char devType[8]; // "WIFI" or "BLE"
    uint32_t timestamp;
};

// Global device registry
ScannedDevice deviceRegistry[MAX_LOGGED_DEVICES];
int deviceCount = 0;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Format byte array to MAC string "XX:XX:XX:XX:XX:XX"
void formatMacAddress(const uint8_t* mac, char* buffer) {
    snprintf(buffer, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Add or update device in registry (Deduplication)
void recordDevice(const char* macStr, int rssi, const char* type) {
    // Check if device already recorded in current window
    for (int i = 0; i < deviceCount; i++) {
        if (strcmp(deviceRegistry[i].macStr, macStr) == 0) {
            // Update highest RSSI (closest distance)
            if (rssi > deviceRegistry[i].rssi) {
                deviceRegistry[i].rssi = rssi;
            }
            deviceRegistry[i].timestamp = millis();
            return;
        }
    }

    // Add new device if buffer has capacity
    if (deviceCount < MAX_LOGGED_DEVICES) {
        strncpy(deviceRegistry[deviceCount].macStr, macStr, 18);
        deviceRegistry[deviceCount].rssi = rssi;
        strncpy(deviceRegistry[deviceCount].devType, type, 8);
        deviceRegistry[deviceCount].timestamp = millis();
        deviceCount++;
    }
}

// ==========================================
// WI-FI PROMISCUOUS MODE CALLBACK
// ==========================================
void wifi_promiscuous_rx_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_ieee80211_mac_header_t* hdr = (wifi_ieee80211_mac_header_t*)pkt->payload;

    int rssi = pkt->rx_ctrl.rssi;
    char srcMacStr[18];

    // Extract Transmitter / Source MAC address
    formatMacAddress(hdr->addr2, srcMacStr);

    // Filter out null/invalid MAC addresses
    if (strcmp(srcMacStr, "00:00:00:00:00:00") != 0 && strcmp(srcMacStr, "FF:FF:FF:FF:FF:FF") != 0) {
        recordDevice(srcMacStr, rssi, "WIFI");
    }
}

// ==========================================
// BLE ADVERTISED DEVICE CALLBACK
// ==========================================
class BLESurroundingCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        std::string address = advertisedDevice.getAddress().toString();
        int rssi = advertisedDevice.getRSSI();

        char macUpper[18];
        strncpy(macUpper, address.c_str(), 18);
        
        // Convert BLE MAC to uppercase
        for (int i = 0; macUpper[i]; i++) {
            macUpper[i] = toupper(macUpper[i]);
        }

        recordDevice(macUpper, rssi, "BLE");
    }
};

// ==========================================
// CORE SNIFFER MODULE API
// ==========================================

void initRFSniffer() {
    Serial.println("[RF_SNIFFER] Initializing Passive RF Sniffer...");

    // 1. Initialize Wi-Fi in Station mode & Enable Promiscuous Sniffing
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_rx_callback);
    
    // 2. Initialize BLE Scanner
    BLEDevice::init("Heimdalls-Watch-Sniffer");
    
    Serial.println("[RF_SNIFFER] Wi-Fi Promiscuous & BLE Scanner Ready.");
}

void executeSurroundingRFScan() {
    deviceCount = 0; // Reset detection window
    Serial.println("\n==========================================");
    Serial.println("[RF_SNIFFER] Starting Surrounding Scan Cycle...");
    Serial.println("==========================================");

    // Step A: Channel Hop across Wi-Fi Channels 1-11
    for (int ch = 1; ch <= 11; ch++) {
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        delay(100); // Dwell time per channel to catch probe requests
    }

    // Step B: Run BLE Active Scan
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new BLESurroundingCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(BLE_SCAN_TIME_SECONDS, false);
    pBLEScan->clearResults(); // Free memory

    // Step C: Print Formatted Forensic Payload (JSON output format)
    Serial.printf("\n[RF_SNIFFER] Scan Complete! Total Nearby RF Devices Identified: %d\n", deviceCount);
    Serial.println("{");
    Serial.println("  \"event\": \"SURROUNDING_RF_SNAPSHOT\",");
    Serial.printf("  \"total_detected\": %d,\n", deviceCount);
    Serial.println("  \"devices\": [");

    for (int i = 0; i < deviceCount; i++) {
        Serial.printf("    {\"mac\": \"%s\", \"type\": \"%s\", \"rssi\": %d}%s\n",
                      deviceRegistry[i].macStr,
                      deviceRegistry[i].devType,
                      deviceRegistry[i].rssi,
                      (i < deviceCount - 1) ? "," : "");
    }
    Serial.println("  ]");
    Serial.println("}\n");
}

// ==========================================
// ARDUINO SETUP & MAIN LOOP
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    initRFSniffer();
}

void loop() {
    // Trigger RF scan every 10 seconds (or on SOS Panic Button Trigger)
    executeSurroundingRFScan();
    delay(10000); 
}