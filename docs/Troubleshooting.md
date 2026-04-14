# Troubleshooting

## "WAITING CAN..."
If the display is stuck on "WAITING CAN...":
1. **VESC Power:** Ensure the motor controller is powered.
2. **VESC ID:** The code is hardcoded to ID 56. If your VESC is different, change `TARGET_VESC_ID` in `main.cpp`.
3. **Wiring:** Check if CAN-H and CAN-L are swapped.
4. **Transceiver:** Ensure GPIO 9 is actually enabling your transceiver.

## BLE Connection drops
If the connection established but then says "Firmware Error":
1. Check the Serial Monitor (115200) to see if "Sent correct FW_VERSION response" is printed.
2. Ensure you are using the latest VESC Tool.
