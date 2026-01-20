#include "radiomodem.h"
#include <logger.h>



// Создаем объект радио в зависимости от типа платы и радиомодуля 
#ifdef ARDUINO_ARCH_ESP32
    SPIClass SPI_MODEM(FSPI); // Используем HSPI для радио в случае ESP32
    // SPIClass SPI_MODEM(HSPI);
    
    #ifdef RADIO_TYPE_SX1278
        // DIO0_PIN используется для прерываний
        Module mod(NSS_PIN, DIO0_PIN, NRST_PIN, DIO1_PIN, SPI_MODEM);
        SX1278 radio(&mod);
    #endif

    #ifdef RADIO_TYPE_SX1268
        // Для SX1268 ПРАВИЛЬНЫЙ порядок по RadioLib:
        // 1. NSS_PIN (CS)
        // 2. DIO1_PIN (IRQ) - Пин прерывания
        // 3. NRST_PIN (Reset) - Пин сброса
        // 4. BUSY_PIN (Busy) - Пин статуса
        // 5. SPI_MODEM - экземпляр SPI
        Module mod(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN, SPI_MODEM);
        SX1268 radio(&mod);
    #endif

#elif defined(ARDUINO_ARCH_ESP8266)
    #define SPI_MODEM SPI // Для ESP8266 используем стандартный объект SPI
    Module mod(NSS_PIN, DIO0_PIN, NRST_PIN, DIO1_PIN, SPI);

    #ifdef RADIO_TYPE_SX1278
        SX1278 radio(&mod);
    #endif
#endif





// Флаг прерывания приема данных
volatile bool receivedFlag = false; 

/**
 * @brief Функция обработки прерывания приема данных радио 
 * 
 */
void IRAM_ATTR setFlag(void) {
    receivedFlag = true;
}


/**
 * ВАЖНО: Если линкер ругается на "undefined reference to MyRadio", 
 * значит объект объявлен как extern, но нигде не создан.
 * Создаем экземпляр менеджера радио здесь:
 */
RadioManager MyRadio;


/**
 * @brief  Функция инициализации радио 
 * 
 * @return true - инициализация успешна
 * @return false - инициализация неуспешна
 */
bool RadioManager::beginRadio() {
    #ifdef ARDUINO_ARCH_ESP32
        //Инициализируем SPI и Reset (оставляем как было, это работает)
        SPI_MODEM.begin(SCK_RADIO, MISO_RADIO, MOSI_RADIO, NSS_PIN);
        
        pinMode(NRST_PIN, OUTPUT);
        digitalWrite(NRST_PIN, LOW);
        delay(20); 
        digitalWrite(NRST_PIN, HIGH);
        delay(50);
                    
    #elif defined(ARDUINO_ARCH_ESP8266)
         // Инициализируем SPI ESP8266
        SPI_MODEM.begin();
    #endif

    //ПОДГОТОВКА ПИТАНИЯ (TCXO) — ДО ОСНОВНОГО BEGIN
    #ifdef RADIO_TYPE_SX1268
        // Мы "говорим" чипу использовать внешний кварц и подать на него 2.4V.
        // Без этого вызов radio.begin ниже вернет -707 (таймаут).
        radio.setTCXO(1.8);
        // radio.setRegulatorDCDC();
        //radio.setRegulatorLDO();
    #endif

    // Настройка ключей антенны
    //radio.setRfSwitchPins(RX_EN_PIN, TX_EN_PIN);

    //ТЕПЕРЬ ЗАПУСКАЕМ ЧИП
    #ifdef RADIO_TYPE_SX1278
        int state = radio.begin(config.frequency, config.bandwidth, config.spreadingFactor, 
                                config.codingRate, config.syncWord, config.outputPower, 
                                config.preambleLength, config.gain);
    #endif
    #ifdef RADIO_TYPE_SX1268
        // Для SX1268: добавляем TCXO Voltage и режим регулятора
        // Именно здесь мы исправляем ту ошибку, которая давала -707
        int state = radio.begin(config.frequency, config.bandwidth, config.spreadingFactor, 
                            config.codingRate, config.syncWord, config.outputPower, 
                            config.preambleLength, config.tcxoVoltage, config.useRegulatorLDO);
    #endif

    if (state == RADIOLIB_ERR_NONE) {

        #ifdef RADIO_TYPE_SX1268
            //radio.setDio2AsRfSwitch(true);
            // Включаем Boosted Gain для лучшего приема (как в Meshtastic)
            radio.setRxBoostedGainMode(true);
        #endif

        #ifdef ARDUINO_ARCH_ESP32
            radio.setRfSwitchPins(RX_EN_PIN, TX_EN_PIN);
        #endif
        radio.setPacketReceivedAction(setFlag);
        radio.setCurrentLimit(config.currentLimit);

        // // Дополнительные настройки из твоего рабочего лога Meshtastic
        // #ifdef RADIO_TYPE_SX1268
        //     radio.setRxBoostedGainMode(RADIOLIB_SX126X_RX_GAIN_BOOSTED);
        // #endif
        
        
        log_radio_event(state, "Radio Init Success");
        startListening(); 
        return true;
    }
    
    // log_radio_event(state, "Radio Init Failed!");
    // Якщо помилка лишилася, виводимо код
    log_radio_event(state, "Radio Init Failed! Error: " + String(state));
    delay(2000);
    return false;
}



/**
 * @brief  Функция запуска режима прослушивания радио
 * 
 */
void RadioManager::startListening() {
    receivedFlag = false;
    radio.startReceive();
}



/**
 * @brief  Проверка, готовы ли данные для чтения 
 * 
 * @return true - данные готовы
 * @return false - данные не готовы
 */
bool RadioManager::isDataReady() {
    return receivedFlag;
}




/**
 * @brief - Функция отправки сообщения через радио 
 * 
 * @param message - сообщение для отправки 
 * @return int - код состояния \ref status_codes 
 */
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


/**
 * @brief  Функция приема сообщения через радио 
 * 
 * @param message - переменная для сохранения принятого сообщения
 * @return int - код состояния \ref status_codes
 */
int RadioManager::receive(String& message) {
    // Читаем данные, которые уже пришли в буфер по прерыванию
    int state = radio.readData(message);
    receivedFlag = false; 
    return state;
}


/**
 * @brief  Функция применения изменений конфигурации радио
 * 
 * @return int - код состояния \ref status_codes
 */
int RadioManager::applyChanges() {
    radio.setCurrentLimit(config.currentLimit);
    return radio.setOutputPower(config.outputPower);
}


/**
 * @brief  Функция получения текущего RSSI и SNR
 * 
 * @return float - значение RSSI или SNR
 */
float RadioManager::getRSSI() { return radio.getRSSI(); }


/**
 * @brief  Функция получения текущего SNR
 * 
 * @return float - значение SNR
 */
float RadioManager::getSNR() { return radio.getSNR(); }