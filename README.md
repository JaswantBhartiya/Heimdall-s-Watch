<div align="center">
  <img src="https://api.iconify.design/material-symbols:shield-person-outline-rounded.svg?color=%23FF2A6D" width="120" alt="Aegis-Wrist Safety Icon" />
  <br><br>
  <img src="https://readme-typing-svg.demolab.com?font=Fira+Code&weight=700&size=27&duration=3000&pause=1000&color=FF2A6D&center=true&vCenter=true&width=750&height=50&lines=Aegis-Wrist%3A+Open-Source+Safety+Wearable;Tamper-Evident+Real-Time+Evidence+Preservation;Discreet+Emergency+Sensing+%26+LTE+Uplink" alt="Typing Effect Headline" />
</div>
<br>

> An open-source, low-cost ($20 BOM) discreet safety wristband engineered to automatically buffer, cryptographically sign, and stream evidence during emergencies—even if the device is forcibly seized or destroyed.

---

## Overview

**Aegis-Wrist** addresses the core vulnerability of traditional emergency alerting tools: preventing an attacker from destroying evidence by capturing and transmitting proof *before* and *during* an incident. 

Built on an **ESP32-S3** microcontroller and **SIM7080G LTE-M modem**, the wristband maintains a continuous 30-second rolling video and audio buffer in volatile PSRAM. Upon manual activation or automatic detection (sudden impact, forced removal), the device cryptographically signs 2-second evidence chunks (SHA-256 + ECDSA) attached with UTC timestamps, GNSS coordinates, and ambient Wi-Fi/BLE signatures, streaming them progressively to cloud endpoints and trusted emergency contacts.

---

## Key Features

- **📷 Pre-Event Memory Buffer:** Maintains a continuous 15–30 second rolling video/audio loop in volatile PSRAM, capturing context immediately *before* an emergency trigger.
- **🆘 Discreet SOS & Anomaly Triggering:** Activates via a flush, silent tactile switch or automated 6-axis IMU detection (impacts, falls, abnormal motion).
- ** Hardware Cryptographic Signing:** Signs every 2-second chunk with SHA-256 hashing and ECDSA signatures bound to the device’s hardware key to establish legal chain-of-custody.
- **📦 Progressive Chunked Upload:** Slices media into 2-second encrypted fragments, transmitting data progressively so partial evidence survives network dropouts or device destruction.
- ** GNSS & Contextual RF Logging:** Attaches precise GPS coordinates along with passive Wi-Fi BSSID and Bluetooth MAC signatures as secondary location verification.
- **Offline Local Backup:** Encrypts and stores unsent evidence chunks on onboard SPI NAND flash during network blackouts, uploading automatically upon reconnection.
- **🔓 Tamper & Removal Detection:** Triggers instant alert transmissions if the strap is cut or optical skin proximity is lost.
- **☀️ Solar-Assisted Charging:** Embedded flexible OPV solar strip along the wristband for trickle-charging power supplementation.

---

## 🛠️ Hardware Bill of Materials (Target: ~$20 BOM)

| Subsystem | Core Component | Role / Specification | Est. Cost (USD) |
| :--- | :--- | :--- | :--- |
| **Main System SoC** | Espressif ESP32-S3-WROOM-1-N8R8 | Dual-core 240MHz MCU, 8MB PSRAM, Wi-Fi/BLE, AES/SHA hardware acceleration | $2.60 |
| **Cellular + GNSS** | SIMCom SIM7080G | Ultra-low power LTE-M / NB-IoT modem + embedded GPS engine | $6.50 |
| **Camera Module** | OmniVision OV2640 / OV5640 | 2MP/5MP DVP image sensor with wide-angle flex cable | $1.80 |
| **Audio Capture** | InvenSense INMP441 | Low-power digital I2S MEMS microphone | $0.50 |
| **Sensors** | ST LSM6DS3TR-C + LTR-303 | 6-axis accelerometer/gyroscope + optical skin proximity sensor | $1.40 |
| **Offline Storage** | Winbond W25N01GV (1Gb) | 128MB SPI NAND Flash for encrypted local caching | $0.80 |
| **Power & Solar** | Consonance CN3163 + OPV Strip | Solar-compatible Li-Po charger IC + 400mAh Li-Po cell | $3.20 |
| **Enclosure & PCB** | Custom 4-Layer FR4 + Silicone | Compact curved rigid PCB layout inside IP67 molded chassis | $3.50 |
| **Total Estimated BOM** | | *Target volume pricing (10,000+ units)* | **~$20.30** |

---

## System Architecture & Data Pipeline

```text
[Camera/Mic Sensors] ---> [ESP32-S3 Rolling PSRAM Buffer (30s)]
                                   |
                       (SOS Switch / Anomaly Trigger)
                                   v
         [Hardware Enclave] ---> [SHA-256 Hashing & ECDSA Signing]
                                   |
                 +-----------------+-----------------+
                 |                                   |
    [SIM7080G LTE-M Uplink]              [128MB SPI NAND Storage]
    (2s Encrypted Chunks)                (Offline Sync on Reconnect)
                 v                                   v
      [Secure Cloud Server] <------------------------+
                 |
      [Trusted Contact Alerts]
```

---

## Repository Directory Structure

```text
womens-safety-wearable/
├── .github/
│   └── workflows/
│       └── build-firmware.yml      # Automated PlatformIO build pipeline
├── firmware/                       # ESP32-S3 C++ Source Code (PlatformIO)
│   ├── include/
│   │   ├── config.h                # Hardware pin definitions & buffer specs
│   │   ├── ram_buffer.h            # Circular PSRAM memory queue implementation
│   │   ├── chunk_signer.h          # SHA-256 & ECDSA signing routines
│   │   ├── lte_streamer.h          # SIM7080G HTTP/MQTT streaming handler
│   │   └── sensor_monitor.h        # IMU threshold & optical tamper logic
│   ├── src/
│   │   ├── main.cpp                # Dual-core tasks & power state machine
│   │   ├── ram_buffer.cpp
│   │   ├── chunk_signer.cpp
│   │   ├── lte_streamer.cpp
│   │   └── sensor_monitor.cpp
│   └── platformio.ini              # Board setup & build flags
├── hardware/                       # Physical Hardware Design
│   ├── schematics/                 # KiCad schematics & PCB layout
│   ├── cad/                        # STEP and STL 3D enclosure models
│   └── bom/
│       └── bill_of_materials.csv   # Itemized component list & suppliers
├── docs/                           # Technical Specifications
│   ├── architecture.md             # System design & memory management
│   ├── threat-model.md             # Security assumptions & attack vectors
│   └── chain-of-custody.md         # Evidence cryptographic verification pipeline
├── .gitignore
├── LICENSE                         # MIT License
└── README.md
```

---

## Quick Start & Development Setup

### Prerequisites

1. Install [VS Code](https://code.visualstudio.com/) and the [PlatformIO IDE Extension](https://platformio.org/).
2. Install [Git](https://git-scm.com/).

### Installation

1. Clone the repository:
   ```bash
   git clone [https://github.com/your-username/womens-safety-wearable.git](https://github.com/your-username/womens-safety-wearable.git)
   cd womens-safety-wearable
   ```

2. Open the `firmware/` directory in VS Code / PlatformIO:
   ```bash
   code firmware
   ```

3. Connect your ESP32-S3 development board via USB-C and run the build:
   ```bash
   pio run -t upload
   ```

4. Open the serial monitor to view diagnostic logs:
   ```bash
   pio device monitor --baud 115200
   ```

---

## Cryptographic Chain of Custody

To guarantee evidence is admissible in legal proceedings, the system enforces strict hardware-level authenticity verification:

1. **Device-Bound Key Creation:** Unique private keys are generated inside the MCU's protected hardware enclave during provisioning.
2. **Chunk Hashing:** Every 2-second video/audio slice, along with UTC timestamp and GPS coordinates, is hashed using SHA-256.
3. **ECDSA Signature:** The digest is signed using the device's private key before upload.
4. **Append-Only Cloud Ledger:** Cloud endpoints verify the signature against the public registry before logging the file manifest into an auditable ledger.

---

## License

Distributed under the **MIT License**. See [`LICENSE`](./LICENSE) for details.
