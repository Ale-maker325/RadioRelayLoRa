#include <Arduino.h>
#include "Button2.h"
#include "settings.h"
#include "radiomodem.h"
#include "output_display.h"
#include "logger.h"
#include "rgb_led.h"



// ОПРЕДЕЛЯЕМ RADIO_NAME: берем из настроек, чтобы логгер знал, кто пишет (TX или RX)
#ifdef TRANSMITTER
  #define BUTTON_DELAY_PRESS 5000  // Задержка удержания кнопки для длинного нажатия (5 секунд)
  String RADIO_NAME = "TX";
  Button2 btn(BUTTON_PIN);
  
  // Прототипы функций
  void handleTap(Button2& b); // Прототип функции обработки нажатия 
  void handleLongPress(Button2& b); // Прототип функции обработки нажатия с удержанием
  bool sendCommandAndWaitAck(String cmd); // Общая функция для отправки с подтверждением
#else
  String RADIO_NAME = "RX";
#endif






void setup()
{
  #ifdef DEBUG_PRINT
    Serial.begin(115200);
    delay(500);
  #endif

  // 1. Инициализация вибромотора (только для TX)
  #if defined(TRANSMITTER) && defined(VIBRO_USED)
    pinMode(VIBRO_PIN, OUTPUT);
    digitalWrite(VIBRO_PIN, HIGH);
    delay(1000); // Тестовая вибрация при включении
    digitalWrite(VIBRO_PIN, LOW);
  #endif

  // 2. Инициализация реле (только для RX)
  #if defined(RECEIVER) && defined(RELAY_USED)
    pinMode(RELAY_PIN, OUTPUT);
    //У ESP32S3 логика реле реализована по-другому чем у ESP8266, - инвертирована
    //Поэтому разлеляем на два варианта
    #if defined(ARDUINO_ARCH_ESP32)
      digitalWrite(RELAY_PIN, LOW);
    #elif defined(ARDUINO_ARCH_ESP8266)
      digitalWrite(RELAY_PIN, HIGH);
    #endif
  #endif
  
  
  
  // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода
  // поэтому этот участок только для ESP32
  #if defined(ARDUINO_ARCH_ESP32)
    // 3. Настройка светодиода (через библиотеку rgb_led.h)
    WriteColorPixel(COLORS_RGB_LED::blue); // В состоянии режима ожидания синий цвет

    // 4. Инициализация дисплея
    #ifdef USE_DISPLAY
      display_init();
    #endif
  #endif

  // 5. Запуск радиомодема
  if (!MyRadio.beginRadio())
  {
    while (1)
    { // Если радио не завелось — мигаем красным
      WriteColorPixel(COLORS_RGB_LED::red);
      delay(100);
      WriteColorPixel(COLORS_RGB_LED::black);
      delay(100);
    }
  }

    #ifdef TRANSMITTER
      btn.setTapHandler(handleTap);
      btn.setLongClickTime(BUTTON_DELAY_PRESS);                 // Устанавливаем порог 5 секунд
      btn.setLongClickDetectedHandler(handleLongPress);   // Обработчик для 5 секунд 
      //log_radio_event(0, "TX System Ready");
      print_log("SYSTEM", "TX Ready: Tap=ON, 5s Hold=OFF");
    #endif

    #ifdef RECEIVER
      // log_radio_event(0, "RX Listening...");
      print_log("SYSTEM", "RX Ready: Waiting for commands...");
      // Модем автоматически уходит в режим прослушки в MyRadio.beginRadio()
    #endif

  // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода (они на одном пине)
  // к тому же пин ESP8266 работает инвертированно по отношению к ESP32, поэтому этот участок только для ESP32
  #if defined(ARDUINO_ARCH_ESP32)
      // Синий цвет — дежурный режим (система готова)
      WriteColorPixel(COLORS_RGB_LED::blue);
  #endif
}












void loop()
{
  #ifdef TRANSMITTER
    btn.loop(); // Опрос кнопки
  #endif

  #ifdef RECEIVER
    // АСИНХРОННЫЙ ПРИЕМ: Проверяем, поймал ли чип сигнал (через прерывание DIO0)
    if (MyRadio.isDataReady()) 
    {
      String rxMessage;
      int state = MyRadio.receive(rxMessage);

      if (state == RADIOLIB_ERR_NONE) 
      {
        print_log("RX", "Received: " + rxMessage);

        if (rxMessage == CMD_RELAY_ON) {
          digitalWrite(RELAY_PIN, LOW); // ВКЛЮЧАЕМ реле постоянно
          delay(50); // ДАЕМ ПЕРЕДАТЧИКУ ВРЕМЯ ПЕРЕКЛЮЧИТЬСЯ НА ПРИЕМ
          MyRadio.send("ACK_ON");        // Подтверждаем включение
          display_print_status("RELAY", "STATUS: ON");
          #ifdef ARDUINO_ARCH_ESP32
            WriteColorPixel(COLORS_RGB_LED::green);
          #endif
        } 
        else if (rxMessage == CMD_RELAY_OFF) {
          digitalWrite(RELAY_PIN, HIGH);  // ВЫКЛЮЧАЕМ реле
          delay(50); // ДАЕМ ПЕРЕДАТЧИКУ ВРЕМЯ ПЕРЕКЛЮЧИТЬСЯ НА ПРИЕМ
          MyRadio.send("ACK_OFF");       // Подтверждаем выключение
          display_print_status("RELAY", "STATUS: OFF");
          #ifdef ARDUINO_ARCH_ESP32
            WriteColorPixel(COLORS_RGB_LED::black);
          #endif
        }
        
        // Возвращаемся в режим прослушивания
        MyRadio.startListening();
        
      }
    }
  #endif
  }





// ЛОГИКА РАБОТЫ ПЕРЕДАТЧИКА (ТХ)
#ifdef TRANSMITTER
  
  /**
 * @brief Обработка короткого нажатия (Включение)
 */
  void handleTap(Button2& b)
  {
    print_log("BUTTON", "Short Press -> Sending ON");
    if (sendCommandAndWaitAck(CMD_RELAY_ON))
    {
      display_print_status("TX OK", "Relay turned ON");
    } else {
      display_print_status("TX FAIL", "No Response");
    }
  }

  /**
 * @brief Обработка долгого нажатия 5 сек (Выключение)
 */
  void handleLongPress(Button2& b)
  {
    print_log("BUTTON", "Long Press (5s) -> Sending OFF");
    if (sendCommandAndWaitAck(CMD_RELAY_OFF))
    {
      display_print_status("TX OK", "Relay turned OFF");
    } else {
      display_print_status("TX FAIL", "No Response");
    }
  }


  /**
 * @brief Логика отправки команды с 3 попытками и ожиданием ответа
 */
bool sendCommandAndWaitAck(String cmd) {
  bool success = false;
  for (int i = 1; i <= 3; i++) {
    print_log("TX", "Attempt " + String(i));
    MyRadio.send(cmd);
    
    // Ждем подтверждение 0.2 секунды
    unsigned long startTime = millis();
    while (millis() - startTime < 200) {
      if (MyRadio.isDataReady()) {
        String ack;
        if (MyRadio.receive(ack) == RADIOLIB_ERR_NONE) {
          if (ack.startsWith("ACK")) { // Проверяем, что это ответ
            success = true;
            break;
          }
        }
      }
    }
    if (success) break;
    delay(100);
  }
  
  if (success) {
    WriteColorPixel(COLORS_RGB_LED::green);
    log_radio_event(0, "DONE! RX CONFIRMED");
  } else {
    WriteColorPixel(COLORS_RGB_LED::red);
    log_radio_event(-1, "FAIL: NO RESPONSE");
  }
  
  MyRadio.startListening(); // Вернуться в режим приема
  return success;
}
#endif









//   bool success = false;
//   String command = String(COMMAND_EN_RELAY);
//   String feedback = "";

//   // Пытаемся отправить команду до 3-х раз
//   for (int attempt = 1; attempt <= 3; attempt++)
//   {
//     // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода (они на одном пине)
//     // к тому же пин ESP8266 работает инвертированно по отношению к ESP32, поэтому этот участок только для ESP32
//     #if defined(ARDUINO_ARCH_ESP32)
//       WriteColorPixel(COLORS_RGB_LED::red); // Красный при передаче
//     #endif
//     log_radio_event(0, "TX Attempt " + String(attempt));

//     // Отправляем сигнал
//     int tx_state = MyRadio.send(command);

//     if (tx_state == RADIOLIB_ERR_NONE)
//     {
//       // Включаем прием сразу после отправки, чтобы поймать ответ
//       MyRadio.startListening();

//       log_radio_event(0, "Wait ACK...");
      
//       // Ожидаем ответа от приемника (таймаут задается в settings.h)
//       unsigned long startWait = millis();
//       while (millis() - startWait < WAITING_RESPONCE)
//       {
//         if (MyRadio.isDataReady()) // Если пришел пакет
//         {
//           int rx_state = MyRadio.receive(feedback);
//           if (rx_state == RADIOLIB_ERR_NONE && feedback == String(ACK_FROM_RECEIVER))
//           {
//             success = true;
//             break; // Успех, выходим из ожидания
//           }
//         }
//         yield(); // Чтобы ESP32 не ушла в перезагрузку по WDT
//       }
//     }

//     if (success) break; // Если получили подтверждение, следующие попытки не нужны
//     delay(100); // Пауза между попытками
//   }
    
//   // Результат операции
//   if (success)
//   {
//     log_radio_event(0, "DONE! RX CONFIRMED");
//     // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода (они на одном пине)
//     // к тому же пин ESP8266 работает инвертированно по отношению к ESP32, поэтому этот участок только для ESP32
//     #if defined(ARDUINO_ARCH_ESP32)
//       WriteColorPixel(COLORS_RGB_LED::green); // Зеленый — успех
//     #endif
//     #ifdef VIBRO_USED
//       digitalWrite(VIBRO_PIN, HIGH);
//       delay(1000);
//       digitalWrite(VIBRO_PIN, LOW);
//     #endif
//   }
//   else
//   {
//     log_radio_event(-1, "FAIL: NO RESPONSE");
//     // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода (они на одном пине)
//     // к тому же пин ESP8266 работает инвертированно по отношению к ESP32, поэтому этот участок только для ESP32
//     #if defined(ARDUINO_ARCH_ESP32)
//       WriteColorPixel(COLORS_RGB_LED::red); // Красный — ошибка связи
//     #endif
//   }

//   // Возвращаемся в дежурный режим через секунду
//   delay(2000);
//   // У ESP8266 нет дисплея и логика работы пина для реле зависит от светодиода (они на одном пине)
//   // к тому же пин ESP8266 работает инвертированно по отношению к ESP32, поэтому этот участок только для ESP32
//   #if defined(ARDUINO_ARCH_ESP32)
//     WriteColorPixel(COLORS_RGB_LED::blue);
//   #endif
  
//   // Возвращаем радио в режим приема
//   MyRadio.startListening();
// }
// #endif