# Hardware Setup & Wiring

## ESP32-C3 Pinout
The VescExpress uses an ESP32-C3. On many variants, the pins are mapped as follows:

### CAN Bus
- **RX (GPIO 0):** Connect to CAN Transceiver R.
- **TX (GPIO 1):** Connect to CAN Transceiver D.
- **EN (GPIO 9):** Connect to Transceiver Standby/Enable. Must be driven **LOW** to operate.

### I2C Display
- **SDA (GPIO 20):** Standard UART-RX pin on 6-pin header.
- **SCL (GPIO 21):** Standard UART-TX pin on 6-pin header.

## Connection to VESC
1. Connect CAN-H and CAN-L from your transceiver to the VESC CAN port.
2. Ensure GND is shared between the ESP32 and the VESC.
3. If using the 6-pin UART port for the display, ensure the VESC's UART is not actively trying to drive those pins.
