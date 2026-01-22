#pragma once
#include <Arduino.h>

class BleManager {
public:
    BleManager() : _isEnabled(false) {}
    
    // Запуск BLE с именем устройства
    void begin(String deviceName);
    
    // Остановка (для экономии энергии)
    void stop();
    
    // Проверка, включен ли модуль
    bool isActive() { return _isEnabled; }
    
    // Фоновая работа (проверка входящих команд)
    void loop();
    
    // Отправка текста в телефон
    void send(String msg);

private:
    bool _isEnabled;
    void handleCommand(String cmd);
};

// Делаем объект MyBLE доступным везде
extern BleManager MyBLE;