# VescExpress Display Functional Specification

## Overview
A PlatformIO-based Arduino project for the VescExpress (ESP32-C3) hardware, providing a real-time dashboard for VESC telemetry via CAN bus and status monitoring for BLE/WiFi.

## Hardware Configuration
- **Board:** ESP32-C3-DevKitM-1
- **Display:** SSD1306 128x64 OLED (I2C)
- **I2C Pins:** 
  - SDA: GPIO 20
  - SCL: GPIO 21
- **CAN Pins:**
  - TX: GPIO 5
  - RX: GPIO 4
- **USB:** CDC on Boot enabled for serial debugging.

## Features
- **Real-time Telemetry:** Displays Voltage, Current, ERPM, and FET Temperature from a target VESC (default ID: 1).
- **Connectivity Status:**
  - **BLE:** Shows if a client (e.g., VESC Tool) is connected via Bluetooth.
  - **WiFi:** Shows current WiFi connection status.
- **Auto-Advertising:** Automatically starts BLE advertising on boot.
- **Heartbeat:** "Online" status detection for VESC data.

## Communication Protocol
- **CAN Bus:** 500kbps (standard VESC bitrate).
- **VESC Packets:**
  - `COMM_GET_VALUES` equivalent via Status Messages 1, 4, and 5.

## UI Layout
- **Status Bar (Top):** WiFi and BLE connectivity indicators.
- **Main Area:** Large voltage readout, current, temperature, and ERPM.
- **Standby Mode:** Displays "WAITING FOR VESC..." if no data is received within 1 second.
