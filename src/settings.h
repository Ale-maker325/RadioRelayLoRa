#pragma once

#include <Arduino.h>

// #define TRANSMITTER     //раскомментировать, если модуль будет использоваться как передатчик
#define RECEIVER      //раскомментировать, если модуль будет использоваться как приёмник

#define DEBUG_PRINT     //раскомментировать для включения отладочного вывода в Serial Monitor
#define USE_DISPLAY     //раскомментировать для использования дисплея
#define FAN_USED        //раскомментировать для использования вентилятора охлаждения при передаче
#define VIBRO_USED      //раскомментировать для использования вибромотора при передаче
#define RELAY_USED      //раскомментировать, если будет использоваться реле

// Выбери один из вариантов твоего модема:
#define RADIO_TYPE_SX1278   // Для E32
// #define RADIO_TYPE_SX1268   // Для E22

// Раскомментируй нужное
#define USE_OLED_SSD1306 
//#define USE_TFT_ST7735

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




#if defined(TRANSMITTER) && defined(VIBRO_USED)
  #define VIBRO_PIN 4
#endif

#ifdef FAN_USED
  #define FUN 5           //Пин вентилятора охлаждения
#endif

#if defined(RECEIVER) && defined(RELAY_USED)
  #define RELAY_PIN 4
#endif

#define LED_PIN 21      //Пин пользовательского светодиода
#define BUTTON_PIN 0   //Пин кнопки пользовательского ввода

//################## пины радио ##################
#ifdef RADIO_TYPE_SX1278

#ifdef USE_OLED_SSD1306
  #define OLED_SDA 12
  #define OLED_SCL 13
#endif

#define MOSI_RADIO 9
#define MISO_RADIO 11
#define SCK_RADIO 8

#define NSS_PIN 7
#define NRST_PIN 3
#define DIO1_PIN 1    //BUSY_PIN
#define DIO0_PIN 2    //IRQ_PIN
#define TX_EN_PIN 6
#define RX_EN_PIN 10

#endif

#ifdef RADIO_TYPE_SX1268

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


