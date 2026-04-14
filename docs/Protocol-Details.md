# VESC Protocol & Handshake

## BLE Handshake
When VESC Tool (Mobile or Desktop) connects via BLE, it immediately sends a `COMM_FW_VERSION` (0) request. 

### Why the "Could not read firmware version" error happens:
VESC Tool expects a very specific packet structure:
1. **Packet ID:** 0
2. **Major Version:** (e.g., 6)
3. **Minor Version:** (e.g., 0)
4. **Hardware Name:** Null-terminated string ("VESC_EXPRESS")
5. **UUID:** 12 bytes
6. **Hardware Type:** 2 (for VESC Express)
7. **Bootloader:** 1 (Presence)

### CRC16-CCITT
The VESC protocol uses a specific CRC16 algorithm. Every packet sent over BLE must have this checksum appended, or the app will reject the data.

```cpp
uint16_t crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0;
    for (uint16_t j = 0; j < len; j++) {
        crc ^= (uint16_t)data[j] << 8;
        for (uint8_t i = 8; i > 0; i--) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}
```
