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

//**************************************************** Параметры радио для компиляции ************************************************
//Задаём параметры конфигурации радиотрансивера 1
#define RADIO_FREQ 460
#define RADIO_BANDWIDTH 125
#define RADIO_SPREAD_FACTOR 9
#define RADIO_CODING_RATE 5
#define RADIO_SYNC_WORD 0x12
#define RADIO_OUTPUT_POWER 5
#define RADIO_CURRENT_LIMIT 140
#define RADIO_PREAMBLE_LENGTH 8
#define RADIO_GAIN 0
#define FAN_THRESHOLD 20 // Порог включения вентилятора (если такой имеется)
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


