#include "ble_manager.h"

// UUID для сервиса UART (стандартные для Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BleManager MyBLE;

// Класс, который слушает события от телефона
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0 && rxValue.length() < 64) { // Ограничим длину команды
            String cmd = "";
            for (int i = 0; i < rxValue.length(); i++) {
                cmd += rxValue[i];
            }
            MyBLE._receivedCommand = cmd;
        }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        // Можно добавить флаг подключения
    };

    void onDisconnect(BLEServer* pServer) {
        // При потере связи запускаем рекламу снова, чтобы можно было переподключиться
        pServer->getAdvertising()->start();
    }
};

void BleManager::begin(String deviceName) {
    if (_isActive) return;

    BLEDevice::init(deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
                                CHARACTERISTIC_UUID_TX,
                                BLECharacteristic::PROPERTY_NOTIFY
                            );
    pTxCharacteristic->addDescriptor(new BLE2902());

    pRxCharacteristic = pService->createCharacteristic(
                                CHARACTERISTIC_UUID_RX,
                                BLECharacteristic::PROPERTY_WRITE
                            );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    
    _isActive = true;
}


void BleManager::stop() {
    if (!_isActive) return;

    // 1. Останавливаем рекламу (Advertising)
    BLEDevice::getAdvertising()->stop();

    // 2. Деинициализируем устройство и освобождаем память
    // Параметр 'true' заставляет Bluetooth стек полностью выгрузиться из памяти
    BLEDevice::deinit(true);

    // 3. Сбрасываем указатели, так как объекты сервера и характеристик удалены внутри deinit
    pServer = nullptr;
    pTxCharacteristic = nullptr;
    pRxCharacteristic = nullptr;

    _isActive = false;
    _receivedCommand = ""; // Чистим буфер на всякий случай
}


void BleManager::loop() {
    // Тут можно обрабатывать переподключения, если нужно
}

bool BleManager::isActive() {
    return _isActive;
}

void BleManager::send(String text) {
    if (_isActive && pTxCharacteristic) {
        pTxCharacteristic->setValue((uint8_t*)text.c_str(), text.length());
        pTxCharacteristic->notify();
    }
}

// --- НОВЫЕ ФУНКЦИИ ---

// Есть ли команда в буфере?
bool BleManager::hasCommand() {
    return _receivedCommand.length() > 0;
}

// Отдать команду и очистить буфер
String BleManager::getCommand() {
    String temp = _receivedCommand;
    _receivedCommand = ""; // Очищаем ящик
    return temp;
}