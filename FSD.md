# VescExpress Display Functional Specification (v2.2.2)

## Hardware Mapping (Verified)
- **Board:** ESP32-C3
- **CAN TX/RX:** GPIO 1 / GPIO 0
- **Transceiver Enable:** GPIO 9 (LOW = Active)
- **Display:** SSD1306 via I2C (GPIO 20/21)

## Implementation Details
### CAN Bus
- **Bitrate:** 500kbps.
- **Target VESC:** ID 56.
- **Bridge ID:** ID 127.

### BLE Stack
- **Service:** Nordic UART Service (NUS).
- **MTU:** Handles standard 20-byte BLE chunks via automatic splitting in `send_ble_packet`.

### UI Logic
- **Library:** U8g2 (Hardware I2C).
- **Page Rotation:** Currently static on Page 1 (SoC/Tacho). Logic structure is ready for multi-page implementation via `current_page` variables.

## Future Roadmap
1.  **Multi-Page UI:** Add Page 2 (Current/Temp) and Page 3 (Trip/Odo).
2.  **Long-Packet Reassembly:** Implement `CAN_PACKET_FILL_RX_BUFFER` buffer for CAN -> BLE direction to support full VESC Tool configuration downloads.
3.  **Auto-ID Discovery:** Re-enable the background scanner to dynamically update `TARGET_VESC_ID`.
