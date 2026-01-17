#ifndef _____rgb_led______
#define _____rgb_led______

#include <Arduino.h>
#include "settings.h"



// Включаем специфичные заголовки только для ESP32
#ifdef ARDUINO_ARCH_ESP32
  #include "soc/soc_caps.h"//Для управління світодіодом
  #include "esp32-hal-rgb-led.h"//Для управління світодіодом
  #define RGB_BRIGHTNESS 10//Регулировка яркости светодиода (max 255)
#endif

      


enum class COLORS_RGB_LED
{
    red,
    green,
    blue,
    black,

};



void WriteColorPixel(COLORS_RGB_LED color)
{
    #ifdef ARDUINO_ARCH_ESP32    
    // ЛОГИКА ДЛЯ ESP32 (RGB NeoPixel)
        switch (color)
        {
        case COLORS_RGB_LED::red :
            neopixelWrite(LED_PIN,0,RGB_BRIGHTNESS,0); // Red
            break;
        case COLORS_RGB_LED::green:
            neopixelWrite(LED_PIN,RGB_BRIGHTNESS,0,0); // Green  
            break;
        case COLORS_RGB_LED::blue:
            neopixelWrite(LED_PIN,0,0,RGB_BRIGHTNESS); // Blue
            break;
        case COLORS_RGB_LED::black:
            neopixelWrite(LED_PIN,0,0,0); // Off / black  
            break;

        default:
            break;
        }
    
    #elif defined(ARDUINO_ARCH_ESP8266)
        
        // ЛОГИКА ДЛЯ ESP8266 (Обычный светодиод/Реле) логику обработки выставляем следующую:
        // HIGH - выключено, LOW - включено (речь идёт о реле , светодид как-бы между прочим будет)
        if (color == COLORS_RGB_LED::black) {
            digitalWrite(LED_PIN, HIGH); // Выключено (подтянуто к плюсу)
        } else {
            digitalWrite(LED_PIN, LOW);  // Включено (любой активный статус)
        }

    #endif
    
}


#endif