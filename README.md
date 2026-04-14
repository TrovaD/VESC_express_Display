# VescExpress Display & Bridge (v2.2.2)

![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blue)
![ESP32-C3](https://img.shields.io/badge/Hardware-ESP32--C3-orange)
![VESC](https://img.shields.io/badge/VESC-6.05%20Compatible-green)

A high-performance real-time telemetry dashboard and **bidirectional BLE-to-CAN bridge** for VESC motor controllers. This project is the result of a systematic hardware discovery process for the VescExpress (ESP32-C3).

## 🚩 Current Project State (Paused)
As of v2.2.2, the project is stable and provides:
- **Bidirectional Bridging:** VESC Tool can discover and configure VESCs on the CAN bus via this ESP32.
- **Real-time UI:** Page 1 is functional, showing SoC (State of Charge) and Tachometer.
- **Verified Hardware:** The exact pinout and transceiver logic for this specific board have been identified.

---

## 🛠 Verified Hardware Configuration
| Component | Pin (GPIO) | Notes |
| :--- | :--- | :--- |
| **CAN RX** | 0 | TWAI RX |
| **CAN TX** | 1 | TWAI TX |
| **CAN EN** | 9 | Must be driven **LOW** |
| **I2C SDA** | 20 | Shared with UART-RX |
| **I2C SCL** | 21 | Shared with UART-TX |

---

## ✨ Key Features
### 1. BLE-to-CAN Bridge
Implemented the VESC fragmentation protocol (`FILL_RX_BUFFER` / `PROCESS_RX_BUFFER`). Large configuration packets from the VESC Tool are split into CAN frames and forwarded to target IDs.

### 2. VESC 6.05 Handshake
Full compatibility with recent VESC Tool versions. The bridge responds locally to `COMM_FW_VERSION` with a complete 16-byte payload and `CRC16-CCITT` checksum to ensure established connections.

### 3. Dashboard (Page 1)
- **SoC Calculation:** Based on a 10S battery profile (configurable).
- **Tachometer:** High-precision reassembly of 32-bit CAN frames.
- **Connection Health:** monitors CAN bus and BLE state.

---

## 💻 Documentation Index
- [Hardware Setup & Wiring](./docs/Hardware-Setup.md)
- [VESC Bridge & Protocol Logic](./docs/Protocol-Details.md)
- [Troubleshooting & Debugging](./docs/Troubleshooting.md)
- [Functional Specification (Technical)](./FSD.md)

---

## ⚖ License
Open-source. Derived from standard VESC communication protocols.
