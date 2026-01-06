#include "radiomodem.h"
#include <logger.h>




#ifdef ARDUINO_ARCH_ESP32
    SPIClass SPI_MODEM(FSPI); //Для ESP32
    // DIO0_PIN (пин 2) используется для прерываний
    Module mod(NSS_PIN, DIO0_PIN, NRST_PIN, DIO1_PIN, SPI_MODEM);
    #ifdef RADIO_TYPE_SX1278
        SX1278 radio(&mod);
    #endif
#elif defined(ARDUINO_ARCH_ESP8266)
    #define SPI_MODEM SPI // Для ESP8266 используем стандартный объект SPI
    Module mod(NSS_PIN, DIO0_PIN, NRST_PIN, DIO1_PIN, SPI);

    #ifdef RADIO_TYPE_SX1278
        SX1278 radio(&mod);
    #endif
#endif





// Флаг прерывания
volatile bool receivedFlag = false; 

// Обработчик прерывания
void IRAM_ATTR setFlag(void) {
    receivedFlag = true;
}

/**
 * ВАЖНО: Если линкер ругается на "undefined reference to MyRadio", 
 * значит объект объявлен как extern, но нигде не создан.
 * Создаем экземпляр менеджера радио здесь:
 */
RadioManager MyRadio;

bool RadioManager::beginRadio() {
    #ifdef ARDUINO_ARCH_ESP32
        //Инициализируем SPI ESP32
        SPI_MODEM.begin(SCK_RADIO, MISO_RADIO, MOSI_RADIO, NSS_PIN);
        radio.setRfSwitchPins(RX_EN_PIN, TX_EN_PIN);
    #elif defined(ARDUINO_ARCH_ESP8266)
         // Инициализируем SPI ESP8266
        SPI_MODEM.begin();
    #endif
    
    // Настройка прерывания на DIO0
    radio.setPacketReceivedAction(setFlag);

    int state = radio.begin(config.frequency, config.bandwidth, config.spreadingFactor, 
                            config.codingRate, config.syncWord, config.outputPower, 
                            config.preambleLength, config.gain);

    if (state == RADIOLIB_ERR_NONE) {
        radio.setCurrentLimit(config.currentLimit);
        log_radio_event(state, "Radio Init Success");
        startListening(); 
        return true;
    }
    
    log_radio_event(state, "Radio Init Failed!");
    return false;
}


/**
 * @brief переходим в режим прослушивания
 * 
 */
void RadioManager::startListening() {
    receivedFlag = false;
    radio.startReceive();
}

bool RadioManager::isDataReady() {
    return receivedFlag;
}

int RadioManager::send(const String& message) {
    #ifdef FAN_USED
    if (config.outputPower >= config.fanThreshold) digitalWrite(FUN, HIGH);
    #endif

    // Использование .c_str() решает проблему "отсутствуют экземпляры перегруженной функции"
    //Библиотека RadioLib имеет несколько вариантов функции transmit. Когда вы передаете const String,
    //компилятор не может автоматически решить, преобразовать ли её в обычную строку или в указатель
    // на символы. Явный вызов message.c_str() превращает объект String в стандартный массив символов char*,
    // который RadioLib понимает однозначно.
    int state = radio.transmit(message.c_str());
    
    log_radio_event(state, "Send: " + message);

    #ifdef FAN_USED
    digitalWrite(FUN, LOW);
    #endif
    
    return state;
}

int RadioManager::receive(String& message) {
    // Читаем данные, которые уже пришли в буфер по прерыванию
    int state = radio.readData(message);
    receivedFlag = false; 
    return state;
}

int RadioManager::applyChanges() {
    radio.setCurrentLimit(config.currentLimit);
    return radio.setOutputPower(config.outputPower);
}

float RadioManager::getRSSI() { return radio.getRSSI(); }
float RadioManager::getSNR() { return radio.getSNR(); }