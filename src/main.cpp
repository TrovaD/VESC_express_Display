#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "driver/twai.h"

/**
 * VescExpress Display & Bridge v2.2.2
 * FULL BLE-to-CAN Bridge for VESC Tool Discovery.
 */

#define CAN_TX_PIN (gpio_num_t)1
#define CAN_RX_PIN (gpio_num_t)0
#define CAN_EN_PIN (gpio_num_t)9
#define I2C_SDA_PIN 20 
#define I2C_SCL_PIN 21 

#define TARGET_VESC_ID 56
#define BRIDGE_VESC_ID 127 // ID assigned to this ESP32 bridge
#define BATTERY_CELLS 10
#define VOLT_MIN (3.2f * BATTERY_CELLS)
#define VOLT_MAX (4.15f * BATTERY_CELLS)

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

// VESC CAN PACKET IDs
#define CAN_PACKET_FILL_RX_BUFFER 5
#define CAN_PACKET_PROCESS_RX_BUFFER 7
#define CAN_PACKET_PROCESS_SHORT_BUFFER 8

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
BLECharacteristic *pTxCharacteristic;
bool ble_connected = false;

struct VescData {
    float voltage = 0.0f;
    float current = 0.0f;
    int32_t erpm = 0;
    int32_t tachometer = 0;
    int soc = 0;
    uint32_t last_msg_ms = 0;
    bool online = false;
} v_data;

// --- VESC CRC16 (CCITT) ---
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

// --- Bridge Logic: CAN -> BLE ---
uint8_t can_rx_buffer[1024];
uint16_t can_rx_ptr = 0;

void send_ble_packet(uint8_t *data, uint32_t len) {
    if (!ble_connected) return;
    int hlen = (len <= 255) ? 2 : 3;
    uint8_t buf[len + hlen + 3];
    int i = 0;
    if (len <= 255) { buf[i++] = 2; buf[i++] = (uint8_t)len; }
    else { buf[i++] = 3; buf[i++] = (uint8_t)(len >> 8); buf[i++] = (uint8_t)(len & 0xFF); }
    memcpy(buf + i, data, len); i += len;
    uint16_t crc = crc16(data, len);
    buf[i++] = (uint8_t)(crc >> 8); buf[i++] = (uint8_t)(crc & 0xFF);
    buf[i++] = 3;
    
    // Split into chunks if necessary for BLE MTU
    for (int j = 0; j < i; j += 20) {
        int chunk = (i - j > 20) ? 20 : (i - j);
        pTxCharacteristic->setValue(buf + j, chunk);
        pTxCharacteristic->notify();
    }
}

// --- Bridge Logic: BLE -> CAN ---
void forward_to_can(uint8_t target_id, uint8_t *data, uint32_t len) {
    if (len <= 6) {
        uint8_t f[8];
        f[0] = BRIDGE_VESC_ID;
        f[1] = 1; // Send response to bridge
        memcpy(f + 2, data, len);
        twai_message_t msg;
        msg.identifier = target_id | (CAN_PACKET_PROCESS_SHORT_BUFFER << 8);
        msg.extd = 1; msg.data_length_code = len + 2;
        memcpy(msg.data, f, msg.data_length_code);
        twai_transmit(&msg, pdMS_TO_TICKS(10));
    } else {
        // Multi-frame fragmentation
        for (uint16_t i = 0; i < len; i += 7) {
            uint8_t chunk = (len - i > 7) ? 7 : (len - i);
            uint8_t f[8];
            f[0] = i; // Offset
            memcpy(f + 1, data + i, chunk);
            twai_message_t msg;
            msg.identifier = target_id | (CAN_PACKET_FILL_RX_BUFFER << 8);
            msg.extd = 1; msg.data_length_code = chunk + 1;
            memcpy(msg.data, f, msg.data_length_code);
            twai_transmit(&msg, pdMS_TO_TICKS(10));
        }
        // Process final buffer
        uint8_t f[6];
        f[0] = BRIDGE_VESC_ID; f[1] = 1; 
        f[2] = len >> 8; f[3] = len & 0xFF;
        uint16_t crc = crc16(data, len);
        f[4] = crc >> 8; f[5] = crc & 0xFF;
        twai_message_t msg;
        msg.identifier = target_id | (CAN_PACKET_PROCESS_RX_BUFFER << 8);
        msg.extd = 1; msg.data_length_code = 6;
        memcpy(msg.data, f, 6);
        twai_transmit(&msg, pdMS_TO_TICKS(10));
    }
}

// --- Protocol Reassembly (BLE side) ---
uint8_t ble_rx_buffer[1024];
uint16_t ble_rx_ptr = 0;
uint16_t ble_expected_len = 0;

void process_ble_packet(uint8_t *data, uint32_t len) {
    uint8_t cmd = data[0];
    if (cmd == 0) { // COMM_FW_VERSION
        uint8_t resp[100]; int i = 0;
        resp[i++] = 0; resp[i++] = 6; resp[i++] = 5;
        const char* hw = "VESC_EXPRESS"; strcpy((char*)&resp[i], hw); i += strlen(hw) + 1;
        uint8_t mac[6]; WiFi.macAddress(mac); memcpy(&resp[i], mac, 6); memset(&resp[i+6], 0, 6); i += 12;
        resp[i++] = 0; resp[i++] = 0; resp[i++] = 2; resp[i++] = 1;
        resp[i++] = 0; resp[i++] = 0; resp[i++] = 0; resp[i++] = 0;
        strcpy((char*)&resp[i], hw); i += strlen(hw) + 1;
        resp[i++] = 0; resp[i++] = 0; resp[i++] = 0; resp[i++] = 0;
        send_ble_packet(resp, i);
    } else if (cmd == 33) { // COMM_FORWARD_CAN
        uint8_t target = data[1];
        forward_to_can(target, data + 2, len - 2);
    } else {
        // Forward all other commands to default target
        forward_to_can(TARGET_VESC_ID, data, len);
    }
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pC) {
        std::string val = pC->getValue();
        for (uint8_t b : val) {
            if (ble_rx_ptr == 0) {
                if (b == 2 || b == 3) ble_rx_buffer[ble_rx_ptr++] = b;
                continue;
            }
            ble_rx_buffer[ble_rx_ptr++] = b;
            if (ble_rx_ptr == 2 && ble_rx_buffer[0] == 2) ble_expected_len = ble_rx_buffer[1] + 5;
            if (ble_rx_ptr == 3 && ble_rx_buffer[0] == 3) ble_expected_len = ((ble_rx_buffer[1] << 8) | ble_rx_buffer[2]) + 6;
            
            if (ble_expected_len > 0 && ble_rx_ptr >= ble_expected_len) {
                uint16_t plen = (ble_rx_buffer[0] == 2) ? ble_rx_buffer[1] : ((ble_rx_buffer[1] << 8) | ble_rx_buffer[2]);
                process_ble_packet(ble_rx_buffer + ble_rx_buffer[0], plen);
                ble_rx_ptr = 0; ble_expected_len = 0;
            }
        }
    }
};

void process_can() {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
        if (!msg.extd) continue;
        uint8_t id = msg.identifier & 0xFF;
        uint8_t cmd = (msg.identifier >> 8) & 0xFF;

        if (id == TARGET_VESC_ID) {
            if (cmd == 0x09) {
                v_data.current = (float)((int16_t)((msg.data[4] << 8) | msg.data[5])) / 10.0f;
                v_data.last_msg_ms = millis();
            } else if (cmd == 0x1B) {
                v_data.tachometer = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                v_data.voltage = (float)((int16_t)((msg.data[4] << 8) | msg.data[5])) / 10.0f;
                if (v_data.voltage <= VOLT_MIN) v_data.soc = 0;
                else if (v_data.voltage >= VOLT_MAX) v_data.soc = 100;
                else v_data.soc = (int)((v_data.voltage - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100.0f);
                v_data.last_msg_ms = millis();
            }
        }
        
        // Response Forwarding (Bridge mode)
        if (id == BRIDGE_VESC_ID) {
            if (cmd == CAN_PACKET_PROCESS_SHORT_BUFFER) {
                send_ble_packet(msg.data + 2, msg.data_length_code - 2);
            }
            // Add long packet reassembly if needed
        }
    }
    v_data.online = (millis() - v_data.last_msg_ms < 1000) && (v_data.last_msg_ms > 0);
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pS) { ble_connected = true; };
    void onDisconnect(BLEServer* pS) { ble_connected = false; BLEDevice::startAdvertising(); }
};

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();
    
    pinMode(CAN_EN_PIN, OUTPUT); digitalWrite(CAN_EN_PIN, LOW);
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();

    BLEDevice::init("VescExpress");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pRxCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
}

void loop() {
    process_can();
    static uint32_t last_ui = 0;
    if (millis() - last_ui > 250) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(0, 10);
        u8g2.printf("ID:%d %s BLE:%s", TARGET_VESC_ID, v_data.online ? "OK" : "NO", ble_connected ? "OK" : "AD");
        u8g2.drawHLine(0, 12, 128);
        if (v_data.online) {
            u8g2.setFont(u8g2_font_logisoso32_tr);
            u8g2.setCursor(0, 44); u8g2.printf("%d%%", v_data.soc);
            u8g2.drawFrame(0, 50, 128, 10);
            int bar_w = (v_data.soc * 124) / 100;
            if (bar_w > 0) u8g2.drawBox(2, 52, bar_w, 6);
            u8g2.setFont(u8g2_font_6x10_tf);
            u8g2.setCursor(0, 64); u8g2.printf("Tacho: %ld", v_data.tachometer);
        } else {
            u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(15, 40, "WAITING DATA");
        }
        u8g2.sendBuffer();
        last_ui = millis();
    }
}
