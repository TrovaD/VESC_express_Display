# VescExpress Display Functional Specification (v2.1.4 - FINAL WORKING)

## Overview
A PlatformIO-based Arduino project for the VescExpress (ESP32-C3) hardware, providing a real-time dashboard for VESC telemetry via CAN bus and a BLE-to-CAN bridge compatible with VESC Tool.

## 🏆 Final Working Configuration (Discovery Phase)
After a systematic hardware scan, the following configuration was verified as functional:

### 1. Hardware Pin Mapping
- **CAN RX:** **GPIO 0** (Verified via signal pulse detection)
- **CAN TX:** **GPIO 1** (Verified via transmission handshake)
- **CAN Transceiver Enable:** **GPIO 9** (Must be driven **LOW**)
- **Display (I2C SDA):** **GPIO 20**
- **Display (I2C SCL):** **GPIO 21**

### 2. Communication Parameters
- **CAN Bitrate:** **500 kbps**
- **VESC ID:** **56** (Locked ID found on the bus)
- **BLE Service:** Nordic UART Service (NUS)

## Critical Fixes Implemented
### Hardware Level
- **Pin Discovery:** Resolved conflicting documentation by running a GPIO scope and pin-pair scan. Standard C3 mappings (4/5 or 18/19) were invalid for this specific board layout.
- **Transceiver Logic:** Identified that the CAN transceiver is in standby by default and requires GPIO 9 to be pulled LOW to active.

### Protocol Level (VESC Tool Compatibility)
- **Firmware Handshake:** Implemented the exact 16-byte response for `COMM_FW_VERSION`.
- **CRC16-CCITT:** Integrated the specific CRC algorithm used by VESC. Without this, the mobile app connects but immediately disconnects with a "Could not read firmware version" error.
- **Packet Framing:** Added support for both short (2-byte) and long (3-byte) VESC packet headers.

## Features
- **Real-time Dashboard:** Displays Voltage, Current, and FET Temperature.
- **Connection Status:** Top bar monitors locked VESC ID, CAN bus health, and BLE state.
- **Persistence:** Configured for the standard VESC 6.0 firmware profile.
