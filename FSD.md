# VescExpress Display Functional Specification (v2.1.3)

## Overview
A PlatformIO-based Arduino project for the VescExpress (ESP32-C3) hardware, providing a real-time dashboard for VESC telemetry via CAN bus and a BLE-to-CAN bridge compatible with VESC Tool.

## Critical Fixes for Stability
The following architectural and protocol fixes were implemented to resolve connectivity and detection issues:

### 1. Hardware Pin Conflict Resolution
- **Display (I2C):** Uses **GPIO 20 (SDA)** and **GPIO 21 (SCL)**. These are the standard UART pins on the 6-pin VESC connector.
- **CAN Bus:** Uses **GPIO 0 (TX)** and **GPIO 1 (RX)**. These are the official VESC Link/Express hardware assignments for the internal TWAI controller.
- **Transceiver Enable:** **GPIO 9** must be driven **LOW** to take the CAN transceiver out of standby/silent mode.

### 2. BLE "Firmware Error" Resolution
The VESC Tool mobile app requires a precise handshake to establish a connection.
- **COMM_FW_VERSION Response:** Implemented the exact 16+ byte payload sequence expected by the VESC protocol, including:
    - Major/Minor Version (6.0)
    - Hardware Name string ("VESC_EXPRESS")
    - 12-byte UUID (MAC address based)
    - Hardware Type byte (set to `2` for VESC Express)
    - Bootloader Presence byte (set to `1`)
- **CRC16-CCITT:** Replaced placeholder checksums with a robust CCITT CRC16 implementation. VESC Tool rejects any packet with an invalid checksum.
- **Packet Framing:** Implemented standard VESC framing using Start Bytes (`2` for short, `3` for long payloads) and the Stop Byte (`3`).

## Hardware Configuration
- **Board:** ESP32-C3-DevKitM-1
- **Partition Table:** `huge_app.csv` (Required for BLE + U8g2)
- **CAN Bitrate:** 500kbps (Standard VESC)

## Features
- **Real-time Telemetry:** Large Voltage readout, Current, and FET Temperature.
- **VESC ID Lock:** Configured to listen to VESC ID **87** by default.
- **Connectivity Dash:** Status bar showing locked ID, CAN status (OK/NO), and BLE status (ADV/OK).
- **Auto-Advertising:** BLE starts automatically on boot as "VescExpress".
