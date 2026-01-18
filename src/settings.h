#pragma once

#include <Arduino.h>


// Выбери один из вариантов твоего модема:
//#define RADIO_TYPE_SX1278   // Для E32
#define RADIO_TYPE_SX1268   // Для E22

#define TRANSMITTER     //раскомментировать, если модуль будет использоваться как передатчик
//#define RECEIVER      //раскомментировать, если модуль будет использоваться как приёмник

#define DEBUG_PRINT     //раскомментировать для включения отладочного вывода в Serial Monitor

//Дисплеи не используются в ESP8266 так как все пины заняты модемом и некоторыми задачами
#if defined(ARDUINO_ARCH_ESP32)
  #define USE_DISPLAY     //раскомментировать для использования дисплея
  //#define FAN_USED        //раскомментировать для использования вентилятора охлаждения при передаче
#endif


#if defined(TRANSMITTER)
  #define VIBRO_USED      //раскомментировать для использования вибромотора при передаче
#elif defined(RECEIVER)
  #define RELAY_USED      //раскомментировать, если будет использоваться реле
#endif



#if defined(ARDUINO_ARCH_ESP32)
  // Раскомментируй нужное для испоьзования дисплея OLED либо TFT
  #define USE_OLED_SSD1306 
  //#define USE_TFT_ST7735
#endif

#define COMMAND_EN_RELAY "CMD_6S_ACTIVATE"  //команда на включение реле
#define ACK_FROM_RECEIVER "ACK_OK"          //подтверждение от приёмника
#define WAITING_RESPONCE 1500                //время ожидания ответа от приёмника когда сигнал отправлен
// Команды управления
#define CMD_RELAY_ON  "RELAY_ON"   // Команда на включение
#define CMD_RELAY_OFF "RELAY_OFF"  // Команда на выключение




//**************************************************** Параметры радио для компиляции ************************************************

/**
 * СПРАВОЧНИК ПАРАМЕТРОВ РАДИО (RadioLib & LoRa)
 * ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * ПАРАМЕТР            | ЗНАЧЕНИЕ (Пример)  | ОПИСАНИЕ И ВЛИЯНИЕ
 * --------------------|-------------------|-----------------------------------------------------------------------------------------------------------------------------
 * Frequency           | 433.0 - 470.0 МГц | Рабочая частота. Должна совпадать у TX и RX.
 * Bandwidth (BW)      | 125.0, 250.0 кГц  | Ширина канала. Меньше BW = выше дальность, но ниже скорость.
 * SpreadingFactor (SF)| 6 - 12            | Коэф. расширения. Выше SF = выше дальность, но пакет летит дольше.
 * CodingRate (CR)     | 5 - 8             | Коррекция ошибок (4/5...4/8). Выше CR = стабильнее связь в помехах.
 * SyncWord            | 0x12 (Private)    | "ID сети". Позволяет не слышать чужие пакеты. 0x34 для LoRaWAN.
 * OutputPower         | 2 - 30 dBm        | Мощность. 
 * PreambleLength      | 8 - 12            | Длина "захода" перед данными. Стандарт - 8.
 * CurrentLimit        | 100.0 - 140.0 мА  | Защита по току. Для мощных модулей ставим максимум (140 для SX126x).
 * TCXO Voltage        | 1.8 или 2.4 В     | Питание кварца. Для Ebyte стабильнее 2.4V. Ошибка -707 если неверно.
 *                     |                   | Согласно даташиту Semtech SX1262/8, встроенный регулятор может выдавать на DIO3 ряд напряжений: 1.6, 1.7, 1.8, 2.2, 2.4, 2.7, 3.0, 3.3 В.
 *                     |                   | Разработчики Ebyte в своих схемах часто ориентируются на питание 1.8В или 2.4В.
 *                     |                   | Почему 2.4V надежнее для Ebyte: Пользователи на GitHub RadioLib и форумах Meshtastic (обсуждая кастомные сборки на Ebyte)
 *                     |                   | часто отмечают, что при 1.8V некоторые экземпляры модулей E22 «заводятся» нестабильно или выдают ошибку -707 (Timeout).
 *                     |                   | Это происходит потому, что при падении напряжения на линии питания или из-за разброса параметров самого кварца, 1.8В
 *                     |                   | оказывается на грани порога запуска. 2.4В — это «золотая середина», которая гарантирует старт генератора на большинстве
 *                     |                   | модулей Ebyte без риска перегрева. 
 *                     |                   | Мнение автора RadioLib (Jan Gisselberg): Он указывает, что если в begin() передать 0 (или неверное напряжение),
 *                     |                   | чип просто не увидит тактового сигнала и зависнет. Он рекомендует проверять даташит конкретного модуля (не чипа),
 *                     |                   | но для большинства SX126x внешних модулей 1.8В — стандарт, а для мощных Ebyte — часто 2.4В. 
 *                     |                   | Итог: Если 1.8V работает стабильно — можно оставить. Если бывают сбои при включении — 2.4V лучший выбор.
 * UseRegulatorLDO     | true / false      | true = LDO (стабильнее), false = DC-DC (экономит заряд батареи).
 * Gain (SX1278)       | 0 (Auto)          | Усиление приемника LNA. 0 - авторегулировка (рекомендуется).
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * Пример настроек для разных сценариев:
 *  - Максимальная дальность: Frequency=460.0, BW=125.0, SF=12, CR=8, SyncWord=0x12, OutputPower=22, PreambleLength=12, CurrentLimit=140, TCXO=2.4V, UseRegulatorLDO=true
 * - Баланс дальности и скорости: Frequency=460.0, BW=250.0, SF=9, CR=5, SyncWord=0x12, OutputPower=14, PreambleLength=8, CurrentLimit=120, TCXO=2.4V, UseRegulatorLDO=false
 * - Высокая скорость передачи: Frequency=460.0, BW=500.0, SF=7, CR=5, SyncWord=0x12, OutputPower=10, PreambleLength=8, CurrentLimit=100, TCXO=1.8V, UseRegulatorLDO=false
 * ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * Что это такое: Cyclic Redundancy Check (Циклический избыточный код). Это математическая контрольная сумма пакета. Как работает: Передатчик считает сумму данных
 * и добавляет 2 байта в конец пакета. Приемник считает сумму полученных данных и сравнивает с этими 2 байтами. Зачем это нужно: В радиоэфире много шума. Без CRC
 * приемник может принять «мусор» (случайный всплеск эфира) за валидный пакет. С включенным CRC библиотека RadioLib просто выдаст ошибку RADIOLIB_ERR_CRC_MISMATCH,
 * и твой код проигнорирует битый пакет. Почему мы не указывали: В RadioLib метод begin() по умолчанию включает CRC. Когда отключать: Только если ты строишь систему,
 * где потеря даже одного бита не важна, или если ты пытаешься поймать пакеты от устройства, которое заведомо шлет данные без CRC (очень редко).
 */

#define FREQUENCY_RADIO 434.125  // Частота радио в МГц

#ifdef RADIO_TYPE_SX1278
  //Задаём параметры конфигурации радиотрансивера 1
  #define RADIO_FREQ FREQUENCY_RADIO
  #define RADIO_BANDWIDTH 125
  #define RADIO_SPREAD_FACTOR 9
  #define RADIO_CODING_RATE 5
  #define RADIO_SYNC_WORD 0x12
  #define RADIO_OUTPUT_POWER 5
  #define RADIO_CURRENT_LIMIT 140
  #define RADIO_PREAMBLE_LENGTH 8
  #define RADIO_GAIN 0
  #define FAN_THRESHOLD 20 // Порог включения вентилятора (если такой имеется)
#endif
#ifdef RADIO_TYPE_SX1268
    // Настройки специфичные для SX1268 (E22 и аналоги)
    #define RADIO_FREQ FREQUENCY_RADIO
    #define RADIO_BANDWIDTH 125.0
    #define RADIO_SPREAD_FACTOR 9
    #define RADIO_CODING_RATE 5
    #define RADIO_SYNC_WORD 0x12     // Для SX126x 0x12 - Private, 0x34 - Public
    #define RADIO_OUTPUT_POWER 22    // Для E22-400M30S можно до 30, но начнем с 22
    #define RADIO_PREAMBLE_LENGTH 8
    #define RADIO_TCXO_VOLTAGE 2.4   // Питание кварца (КРИТИЧНО для SX126x)
    #define RADIO_USE_LDO false      // false = использовать DC-DC (лучше для E22)
    #define RADIO_CURRENT_LIMIT 140  // Лимит тока в мА
    #define FAN_THRESHOLD 20             // Порог включения вентилятора (если такой имеется)
#endif
//**************************************************** Параметры радио для компиляции ************************************************


#ifdef TRANSMITTER 
  extern String RADIO_NAME; // Только объявление
#endif

#ifdef RECEIVER
  extern String RADIO_NAME; // Только объявление
#endif




// ################## ПИНЫ ПЕРИФЕРИИ ##################

#if defined(ARDUINO_ARCH_ESP32)
  // --- Настройки для ESP32-S3 ---
  
  //****************  Для варианта радио старого образца Mesh_Zero_v1 на модеме E32-400M33S  *********************//
  #ifdef RADIO_TYPE_SX1278

    #if defined(TRANSMITTER) && defined(VIBRO_USED)
      #define VIBRO_PIN 4
    #endif

    #ifdef FAN_USED
      #define FUN 5           //Пин вентилятора охлаждения
    #endif

    #if defined(RECEIVER) && defined(RELAY_USED)
      #define RELAY_PIN 4
    #endif

    #ifdef USE_OLED_SSD1306
      #define OLED_SDA 12
      #define OLED_SCL 13
    #endif
    
    #define LED_PIN 21      // Пин RGB светодиода
    #define BUTTON_PIN 0    // Пин кнопки

    #define MOSI_RADIO 9
    #define MISO_RADIO 11
    #define SCK_RADIO 8

    #define NSS_PIN 7
    #define NRST_PIN 3
    #define DIO0_PIN 2    //IRQ_PIN
    #define DIO1_PIN 1    //BUSY_PIN
    #define TX_EN_PIN 6
    #define RX_EN_PIN 10
  
  #endif
  //**************** Конец варианта радио старого образца Mesh_Zero_v1 на модеме E32-400M33S  *********************//
  
  //****************   Для варианта радио нового образца Mesh_Zero_v2 на модеме E22-400M30S ************************//
  #ifdef RADIO_TYPE_SX1268
    
    #if defined(TRANSMITTER) && defined(VIBRO_USED)
      #define VIBRO_PIN 18
    #endif

    //#ifdef FAN_USED
      //#define FUN 5           //Пин вентилятора охлаждения
    //#endif

    #if defined(RECEIVER) && defined(RELAY_USED)
      #define RELAY_PIN 4
    #endif

    #define LED_PIN 21      // Пин RGB светодиода
    #define BUTTON_PIN 0    // Пин кнопки


    #ifdef USE_OLED_SSD1306
      #define OLED_SDA 3
      #define OLED_SCL 5
    #endif

    #define MOSI_RADIO 8
    #define MISO_RADIO 9
    #define SCK_RADIO 7

    #define NSS_PIN 13
    #define NRST_PIN 12
    #define BUSY_PIN 11
    #define DIO1_PIN 10
    #define TX_EN_PIN 2
    #define RX_EN_PIN 1

  #endif
  //**************** Конец варианта радио нового образца Mesh_Zero_v2 на модеме E22-400M30S ************************//




//Настройки для контроллера на ESP8266 (может использоваться как в качестве приёмника, так и передатчика но без дисплея)
#elif defined(ARDUINO_ARCH_ESP8266)

  // --- Настройки для ESP8266 (схема с реле на D0 в случае приёмника, либо кнопкой на D3 в случае передатчика)
  #define LED_PIN D0       // Он же пин управления РЕЛЕ и СВЕТОДИОДОМ
  #if defined(RECEIVER)
    #define RELAY_PIN D0     // Дублируем для ясности кода
  #elif defined(TRANSMITTER)
    #define BUTTON_PIN D3    // Пин кнопки (для передатчика, если нужен передатчик)
  #endif
  
  // Пины SPI для ESP8266 (стандартные)
  #define NSS_PIN SS       // Обычно это GPIO15
  #define DIO0_PIN D2      // GPIO4
  #define DIO1_PIN D1      // GPIO5
  #define NRST_PIN D4      // GPIO2
  
  //Пины для подключения SPI модема используют стандартный для ESP8266
  //набор пинов: MOSI (GPIO 13) - D7 ,MISO (GPIO 12) - D6 ,SCK (GPIO 14) - D5
  // #define SCK_RADIO D5
  // #define MISO_RADIO D6
  // #define MOSI_RADIO D7

#endif


