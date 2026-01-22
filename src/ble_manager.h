#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // Нужен для уведомлений (notify)

class BleManager {
public:
    void begin(String deviceName);
    void stop();      // <--- ДОБАВЛЕНО: Правильное выключение BLE
    void loop(); // Обработчик в главном цикле
    bool isActive(); // Включен ли блютуз вообще
    void send(String text); // Отправка ответа на телефон

    // НОВЫЕ ФУНКЦИИ ДЛЯ "ПОЧТОВОГО ЯЩИКА"
    bool hasCommand();        // Проверка: пришла ли новая команда?
    String getCommand();      // Забрать команду и очистить ящик

private:
    BLEServer* pServer = nullptr;
    BLECharacteristic* pTxCharacteristic = nullptr;
    BLECharacteristic* pRxCharacteristic = nullptr;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;
    bool _isActive = false;

    // Переменная для хранения команды, пока main её не заберет
    String _receivedCommand = ""; 

    // Дружим с классом-колбэком, чтобы он мог писать в _receivedCommand
    friend class MyCallbacks; 

    unsigned long _startTime = 0;
    bool _isAuthorized = false;
    String _currentPassword = "";
};

extern BleManager MyBLE;

#endif