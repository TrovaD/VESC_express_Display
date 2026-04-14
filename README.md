# VescExpress Display & Bridge

![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blue)
![ESP32-C3](https://img.shields.io/badge/Hardware-ESP32--C3-orange)
![VESC](https://img.shields.io/badge/VESC-Compatible-green)

A high-performance real-time telemetry dashboard and BLE-to-CAN bridge for VESC motor controllers, specifically optimized for the **VescExpress (ESP32-C3)** hardware.

## 🚀 Overview
This project transforms an ESP32-C3 into a dedicated dashboard for your e-vehicle. it provides:
- **Live Telemetry:** Voltage, Current, and Temperature tracking.
- **BLE Bridge:** Connect your phone's VESC Tool directly to the VESC via this dashboard.
- **Plug & Play:** Auto-detects and locks onto active VESC IDs.

---

## 🛠 Hardware Configuration
The pinout for this project was verified through systematic hardware scanning to ensure compatibility with custom VescExpress board layouts.

| Component | Pin (GPIO) | Notes |
| :--- | :--- | :--- |
| **CAN RX** | 0 | Internal TWAI RX |
| **CAN TX** | 1 | Internal TWAI TX |
| **CAN EN** | 9 | Driven **LOW** to enable transceiver |
| **I2C SDA** | 20 | Shared with UART-RX pin |
| **I2C SCL** | 21 | Shared with UART-TX pin |

**Display:** SSD1306 128x64 OLED via I2C.

---

## ✨ Key Features
### 1. VESC Tool Compatibility
Fixed the common "Could not read firmware version" error by implementing the full VESC handshake protocol and a hardware-accurate CRC16-CCITT checksum.

### 2. Intelligent CAN Bus Monitoring
- **Bitrate:** 500kbps (Standard).
- **Auto-Detection:** Automatically identifies the VESC ID (e.g., ID 56) by listening to status frames.
- **Bus Health:** Real-time monitoring of CAN state and error counters.

---

## 💻 Installation
1. Install [PlatformIO](https://platformio.org/).
2. Clone this repository:
   ```bash
   git clone https://github.com/TrovaD/VESC_express_Display.git
   ```
3. Open the project in VSCode/PlatformIO.
4. Connect your VescExpress via USB.
5. Click **Upload**.

---

## 📖 Wiki & Documentation
Detailed documentation is available in the `docs/` folder:
- [Hardware Setup & Wiring](./docs/Hardware-Setup.md)
- [VESC Protocol Handshake Details](./docs/Protocol-Details.md)
- [Troubleshooting](./docs/Troubleshooting.md)

---

## ⚖ License
This project is open-source. See the official VESC Express repository for upstream protocol licenses.
