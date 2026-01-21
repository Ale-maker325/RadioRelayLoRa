#include "output_display.h"
#include "settings.h"

// --- Секция для OLED SSD1306 ---
#ifdef USE_OLED_SSD1306
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #include <Wire.h>

  Adafruit_SSD1306 display(128, 64, &Wire, -1);

  bool isDisplayReady = false; // Глобальный флаг правильности инициализации дисплея

  /**
   * @brief Инициализация дисплея OLED
   * 
   */
  void display_init()
  {
    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
      isDisplayReady = false; // Экран не найден
      #ifdef DEBUG_PRINT
        Serial.println("_____ display not found_____ !!!!");
      #endif
      return;
    }else{
      isDisplayReady = true;
      #ifdef DEBUG_PRINT
        Serial.print("display found at 0x3C : ");
        Serial.print("SDA = ");
        Serial.print(OLED_SDA);
        Serial.print(", SCL = ");
        Serial.println(OLED_SCL);
      #endif
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
  }

  void display_print_status(String status, String message)
  {
    if (!isDisplayReady) return; // Если экрана нет, ничего не делаем и не тратим время
    display.clearDisplay();
    display.setCursor(0,5);
    display.println(status);
    display.println("---------");
    display.println(message);
    display.display();
  }

  void display_print_status(int x, int y, String status, String message)
  {
    if (!isDisplayReady) return; // Если экрана нет, ничего не делаем и не тратим время
    display.clearDisplay();
    display.setCursor(x, y);
    display.println(status);
    display.println("---------");
    display.println(message);
    display.display();
  }

  void display_clear()
  {
    if (!isDisplayReady) return; // Если экрана нет, ничего не делаем и не тратим время
    display.clearDisplay();
    display.display();
  }

// --- Секция для TFT (пример на будущее) ---
#elif defined(USE_TFT_ST7735)
  #include <Adafruit_ST7735.h>
  // Тут будет инициализация TFT...
  void display_init() { /* код для TFT */ }
  void display_print_status(String s, String m) { /* код для TFT */ }
  void display_clear() { /* код для TFT */ }

// --- Если дисплей не выбран ---
#else
  void display_init() {}
  void display_print_status(String s, String m) {}
  void display_clear() {}
#endif