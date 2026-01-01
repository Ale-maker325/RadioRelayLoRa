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
    uint16_t currentLimit = RADIO_CURRENT_LIMIT;
    int16_t preambleLength = RADIO_PREAMBLE_LENGTH;
    uint8_t gain = RADIO_GAIN;
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

    float getRSSI(); 
    float getSNR();
};

extern RadioManager MyRadio;