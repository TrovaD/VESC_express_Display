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
 * VescExpress Display & Bridge v2.1.3
 * Targeted Fix: Correcting COMM_FW_VERSION response and CRC16.
 */

// --- GPIO Pins (ESP32-C3) ---
#define CAN_TX_PIN (gpio_num_t)0
#define CAN_RX_PIN (gpio_num_t)1
#define CAN_EN_PIN (gpio_num_t)9
#define I2C_SDA_PIN 20 
#define I2C_SCL_PIN 21 

#define TARGET_VESC_ID 87

// --- BLE NUS UUIDs ---
#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
BLECharacteristic *pTxCharacteristic;
bool ble_connected = false;

struct VescData {
    float voltage = 0.0f;
    float current = 0.0f;
    int32_t erpm = 0;
    float temp_fet = 0.0f;
    uint32_t last_msg_ms = 0;
    bool online = false;
} v_data;

// --- Correct VESC CRC16 (CCITT) ---
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

void send_ble_packet(uint8_t *data, uint32_t len) {
    if (!ble_connected) return;
    int header_len = (len <= 255) ? 2 : 3;
    uint32_t total_len = len + header_len + 3; // Header + Payload + CRC(2) + Stop(1)
    uint8_t buf[total_len];
    int ind = 0;
    
    if (len <= 255) {
        buf[ind++] = 2; 
        buf[ind++] = (uint8_t)len;
    } else {
        buf[ind++] = 3;
        buf[ind++] = (uint8_t)(len >> 8);
        buf[ind++] = (uint8_t)(len & 0xFF);
    }
    
    memcpy(buf + ind, data, len);
    ind += len;
    
    uint16_t crc = crc16(data, len);
    buf[ind++] = (uint8_t)(crc >> 8);
    buf[ind++] = (uint8_t)(crc & 0xFF);
    buf[ind++] = 3; // Stop byte

    pTxCharacteristic->setValue(buf, ind);
    pTxCharacteristic->notify();
}

void init_can() {
    pinMode(CAN_EN_PIN, OUTPUT);
    digitalWrite(CAN_EN_PIN, LOW);
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();
}

void process_can() {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
        if (!msg.extd) continue;
        uint8_t id = msg.identifier & 0xFF;
        if (id != TARGET_VESC_ID) continue;
        uint8_t cmd = (msg.identifier >> 8) & 0xFF;
        switch (cmd) {
            case 0x09:
                v_data.erpm = (int32_t)((msg.data[0] << 24) | (msg.data[1] << 16) | (msg.data[2] << 8) | msg.data[3]);
                v_data.current = (float)((int16_t)((msg.data[4] << 8) | msg.data[5])) / 10.0f;
                v_data.last_msg_ms = millis();
                break;
            case 0x1B:
                v_data.voltage = (float)((int16_t)((msg.data[4] << 8) | msg.data[5])) / 10.0f;
                v_data.last_msg_ms = millis();
                break;
            case 0x10:
                v_data.temp_fet = (float)((int16_t)((msg.data[0] << 8) | msg.data[1])) / 10.0f;
                break;
        }
    }
    v_data.online = (millis() - v_data.last_msg_ms < 1000) && (v_data.last_msg_ms > 0);
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { ble_connected = true; };
    void onDisconnect(BLEServer* pServer) { ble_connected = false; BLEDevice::startAdvertising(); }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() < 3) return;
        uint8_t *data = (uint8_t*)value.data();
        uint8_t cmd = (data[0] == 2) ? data[2] : data[3];

        if (cmd == 0) { // COMM_FW_VERSION
            uint8_t resp[100];
            int i = 0;
            resp[i++] = 0;    // COMM_FW_VERSION
            resp[i++] = 6;    // Major
            resp[i++] = 0;    // Minor
            const char* hw = "VESC_EXPRESS";
            strcpy((char*)&resp[i], hw); i += strlen(hw) + 1;
            uint8_t mac[6]; WiFi.macAddress(mac);
            memcpy(&resp[i], mac, 6); memset(&resp[i+6], 0, 6); i += 12;
            resp[i++] = 0;    // Pairing
            resp[i++] = 0;    // Test version
            resp[i++] = 2;    // HW Type: VESC Express
            resp[i++] = 1;    // Bootloader Presence (1 = Yes)
            send_ble_packet(resp, i);
            Serial.println("BLE: Sent correct FW_VERSION response");
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();
    init_can();
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
        u8g2.printf("ID:%d CAN:%s BLE:%s", TARGET_VESC_ID, v_data.online ? "OK" : "NO", ble_connected ? "OK" : "AD");
        u8g2.drawHLine(0, 12, 128);
        if (v_data.online) {
            u8g2.setFont(u8g2_font_logisoso24_tr);
            u8g2.setCursor(0, 42); u8g2.print(v_data.voltage, 1); u8g2.print("V");
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(0, 58); u8g2.printf("%.1fA  %.0fC FET", v_data.current, v_data.temp_fet);
        } else {
            u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(10, 40, "WAITING CAN...");
        }
        u8g2.sendBuffer();
        last_ui = millis();
    }
}
