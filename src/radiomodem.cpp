#include "radiomodem.h"
#include <logger.h>




#ifdef ARDUINO_ARCH_ESP32
    // SPIClass SPI_MODEM(FSPI); //Для ESP32
    SPIClass SPI_MODEM(HSPI);
    
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
        //Инициализируем SPI
        SPI_MODEM.begin(SCK_RADIO, MISO_RADIO, MOSI_RADIO, NSS_PIN);

        #ifdef RADIO_TYPE_SX1268
            //РУЧНОЙ СБРОС — КРИТИЧЕСКИ ВАЖНО для SX1268
            //Без этого пин BUSY может быть заблокирован в HIGH, и будет ошибка -707
            pinMode(NRST_PIN, OUTPUT);
            digitalWrite(NRST_PIN, LOW);
            delay(20); 
            digitalWrite(NRST_PIN, HIGH);
            delay(50);
        #endif
            
    #elif defined(ARDUINO_ARCH_ESP8266)
         // Инициализируем SPI ESP8266
        SPI_MODEM.begin();
    #endif

    //ПОДГОТОВКА ПИТАНИЯ (TCXO) — ДО ОСНОВНОГО BEGIN
    #ifdef RADIO_TYPE_SX1268
        // Мы "говорим" чипу использовать внешний кварц и подать на него 2.4V.
        // Без этого вызов radio.begin ниже вернет -707 (таймаут).
        radio.setTCXO(2.4); 
    #endif

    //ТЕПЕРЬ ЗАПУСКАЕМ ЧИП
    int state = radio.begin(config.frequency, config.bandwidth, config.spreadingFactor, 
                            config.codingRate, config.syncWord, config.outputPower, 
                            config.preambleLength, config.gain);

    if (state == RADIOLIB_ERR_NONE) {

        // ПРАВИЛЬНАЯ ИНИЦИАЛИЗАЦИЯ ДЛЯ SX1268 (E22)
        // Сначала вызываем базовый begin БЕЗ параметров, чтобы просто "разбудить" чип
        // Но перед этим ОБЯЗАТЕЛЬНО настраиваем TCXO, так как без него он не ответит.
        #ifdef RADIO_TYPE_SX1268
            radio.setDio2AsRfSwitch(true);
            // Включаем Boosted Gain для лучшего приема (как в Meshtastic)
            radio.setRxBoostedGainMode(true);
        #endif

        // Только после успешного begin настраиваем обвязку:
        // Настройка ключей антенны
        radio.setRfSwitchPins(RX_EN_PIN, TX_EN_PIN);

        // Настройка прерывания
        radio.setPacketReceivedAction(setFlag);

        // Дополнительные настройки из твоего рабочего лога Meshtastic
        #ifdef RADIO_TYPE_SX1268
            radio.setRxBoostedGainMode(RADIOLIB_SX126X_RX_GAIN_BOOSTED);
        #endif
        
        
        radio.setCurrentLimit(config.currentLimit);
        log_radio_event(state, "Radio Init Success");
        delay(500);
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