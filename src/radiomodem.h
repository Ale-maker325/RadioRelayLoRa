#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include "settings.h"

struct LORA_CONFIGURATION {
    float frequency = RADIO_FREQ;
    float bandwidth = RADIO_BANDWIDTH;
    uint8_t spreadingFactor = RADIO_SPREAD_FACTOR;
    uint8_t codingRate = RADIO_CODING_RATE;
    uint8_t syncWord = RADIO_SYNC_WORD;
    int8_t outputPower = RADIO_OUTPUT_POWER;
    float currentLimit = RADIO_CURRENT_LIMIT; // Изменено на float для совместимости
    uint16_t preambleLength = RADIO_PREAMBLE_LENGTH;
    
    #ifdef RADIO_TYPE_SX1278
        uint8_t gain = RADIO_GAIN;
    #endif

    #ifdef RADIO_TYPE_SX1268
        float tcxoVoltage = RADIO_TCXO_VOLTAGE; // ЭТОГО НЕ ХВАТАЛО
        bool useRegulatorLDO = RADIO_USE_LDO;   // ЭТОГО НЕ ХВАТАЛО
    #endif

    int8_t fanThreshold = 20; 
};




class RadioManager {
public:
    RadioManager() = default;
    LORA_CONFIGURATION config;
    
    bool beginRadio();
    
    // const String& позволяет передавать String("текст") без ошибок lvalue
    int send(const String& message); 
    
    int receive(String& message);
    int applyChanges();

    // Асинхронные методы (прерывания)
    void startListening(); 
    bool isDataReady();

    /**
     * @brief - Функция отправки команды и ожидания подтверждения 
     * 
     * @param cmd - команда для отправки
     * @param onTick - указатель на функцию (например, btn.loop), чтобы не вешать процессор
     */
    bool sendCommandAndWaitAck(String cmd, void (*onTick)() = nullptr);

    float getRSSI(); 
    float getSNR();

    // Флаги (чек-боксы) нашего кода
    bool isProcessing = false; // "Шлагбаум": если true, значит мы сейчас ждем ответ от радио и кнопку нажимать бесполезно
    bool relayIsOn = false;    // Наше мнение о том, в каком состоянии сейчас реле
    bool rxOnline = false;     // Связь: true, если приемник хоть раз ответил на команду успешно
};

extern RadioManager MyRadio;