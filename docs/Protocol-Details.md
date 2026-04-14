# VESC Bridge & Protocol Logic

## BLE-to-CAN Bridging
The core of this project is the ability to tunnel standard VESC packets over the CAN bus. This allows the mobile app to communicate with controllers that don't have built-in Bluetooth.

### Fragmentation (BLE -> CAN)
When a large packet (e.g., > 6 bytes) is received over BLE, it must be fragmented:
1.  **Fragments:** Sent using `CAN_PACKET_FILL_RX_BUFFER` (ID 5). The first byte of each CAN frame is the buffer offset.
2.  **Execution:** After all fragments are sent, a `CAN_PACKET_PROCESS_RX_BUFFER` (ID 7) frame is sent. This contains the sender ID, the response type, the total length, and a CRC16 of the payload.

### Reassembly (CAN -> BLE)
Currently, version 2.2.2 handles single-frame responses from the VESC. For full support of large configuration downloads, a reassembly buffer for `CAN_PACKET_FILL_RX_BUFFER` frames coming *from* the VESC should be implemented in the future.

## VESC Handshake (v6.05)
The VESC Tool requires the following sequence to establish a connection:
1.  **Request:** `COMM_FW_VERSION` (Byte 0)
2.  **Response:**
    - `[0]` - Packet ID
    - `[1]` - Major (6)
    - `[2]` - Minor (5)
    - `[...]` - HW Name (Null-terminated)
    - `[+12]` - UUID
    - `[+1]` - Pairing Status
    - `[+1]` - Test Version
    - `[+1]` - HW Type (2 = Vesc Express)
    - `[+1]` - Bootloader Present (1)
    - `[...]` - (Additional fields for 6.05 compatibility)
