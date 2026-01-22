#include "ble_manager.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "logger.h"

// Создаем глобальный объект
BleManager MyBLE;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
String rxBuffer = "";

// Стандартные UUID для сервиса UART (как в приложении Serial Bluetooth Terminal)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Обработка подключения/отключения
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        print_log("[BLE]", "Phone Connected");
    };
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        print_log("[BLE]", "Phone Disconnected");
        // Перезапускаем рекламу, чтобы можно было переподключиться
        pServer->getAdvertising()->start();
    }
};

// Обработка входящих данных от телефона
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            for (int i = 0; i < value.length(); i++) {
                rxBuffer += value[i];
            }
        }
    }
};

void BleManager::begin(String deviceName) {
    if (_isEnabled) return;

    BLEDevice::init(deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Характеристика для передачи данных В ТЕЛЕФОН (TX)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Характеристика для приема данных ИЗ ТЕЛЕФОНА (RX)
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    
    _isEnabled = true;
    print_log("[BLE]", "BLE UART Started on S3");
}

void BleManager::loop() {
    if (!_isEnabled) return;

    if (rxBuffer.length() > 0) {
        // Если пришла команда (заканчивается на новую строку или просто текст)
        handleCommand(rxBuffer);
        rxBuffer = ""; 
    }
}

void BleManager::send(String msg) {
    if (_isEnabled && deviceConnected) {
        pTxCharacteristic->setValue(msg.c_str());
        pTxCharacteristic->notify();
    }
}

void BleManager::handleCommand(String cmd) {
    cmd.trim();
    print_log("[BLE CMD]", cmd);
    // Тут можно добавить логику управления через BLE
}

void BleManager::stop() {
    // BLE на S3 лучше не выключать полностью "на лету", 
    // но если нужно - просто прекращаем рекламу
    _isEnabled = false;
}