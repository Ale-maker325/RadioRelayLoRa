#include <Arduino.h>
#include "Button2.h"
#include "settings.h"
#include "radiomodem.h"
#include "output_display.h"
#include "logger.h"
#include "rgb_led.h"

// Библиотека для работы с энергонезависимой памятью ESP32 встроенная в Arduino Core
#if defined(ARDUINO_ARCH_ESP32)
  #include <Preferences.h> 
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <EEPROM.h> // Добавляем EEPROM для 8266
#endif

// ОПРЕДЕЛЯЕМ RADIO_NAME: берем из настроек, чтобы логгер знал, кто пишет (TX или RX)
#ifdef TRANSMITTER
  String RADIO_NAME = "TX";
  // Параметры по порядку: 
  // 1. BUTTON_PIN - номер твоего пина
  // 2. INPUT_PULLUP - режим работы (уже выбран по умолчанию)
  // 3. true - означает, что кнопка замыкается на "землю" (Active Low)
  Button2 btn(BUTTON_PIN, INPUT_PULLUP, true);
  
  // Объект памяти только для ESP32
  #if defined(ARDUINO_ARCH_ESP32)
    Preferences pref;
  #endif

  // Глобальные переменные состояния  
  bool isProcessing = false; // Флаг для защиты от повторных нажатий во время работы
  bool relayIsOn = false;     // Текущее состояние реле (подгрузится из памяти)


  
  // Прототипы функций
  void handleTap(Button2& b); // Прототип функции обработки однократного нажатия кнопки
  void handleDoubleClick(Button2& b); // Прототип функции обработки двойного нажатия кнопки
  bool sendCommandAndWaitAck(String cmd); // Умная функция с таймаутом для отправки с подтверждением 
#else
  String RADIO_NAME = "RX";
  bool relayIsOn = false;
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
      print_log("[ERROR] ", "Radio Init Failed!");
      display_print_status("[ERROR] ", "Radio Init Failed!");
    }
  }

  // 6. Настройка кнопки (только для TX)
  #ifdef TRANSMITTER
    #if defined(ARDUINO_ARCH_ESP32)
      //Инициализация энергонезависимой памяти (NVS) "relay-app" - имя хранилища, false - режим чтение/запись
      pref.begin("relay-app", false);
      //Загружаем сохраненное состояние. Если данных нет (первый запуск), ставим false (OFF)
      relayIsOn = pref.getBool("state", false);
    #endif

    // --- НОВАЯ ФИТЧА: Опрос реального состояния при включении ---
    #ifdef RELAY_GET_STATUS
      print_log("[SYSTEM] ", "Checking RX status...");
      String statusResp;
      MyRadio.send(CMD_GET_STATUS);
      MyRadio.startListening();
      
      unsigned long startQ = millis();
      while (millis() - startQ < 1000) { // Ждем ответ 1 секунду
        if (MyRadio.isDataReady()) {
          if (MyRadio.receive(statusResp) == RADIOLIB_ERR_NONE) {
            if (statusResp == ACK_RELAY_IS_ON) {
                relayIsOn = true;
                #if defined(ARDUINO_ARCH_ESP32)
                  pref.putBool("state", true);
                #endif
                break;
            } else if (statusResp == ACK_RELAY_IS_OFF) {
                relayIsOn = false;
                #if defined(ARDUINO_ARCH_ESP32)
                  pref.putBool("state", false);
                #endif
                break;
            }
          }
        }
      }
    #endif
    // --- Конец новой фитчи ---
    
    //Настраиваем обработчики по совету разработчика библиотеки ClickHandler четко разделяет одиночное и двойное нажатие
    btn.setTapHandler(handleTap);
    btn.setDoubleClickTime(500); // Устанавливаем время для распознавания двойного клика (500 мс)
    btn.setDoubleClickHandler(handleDoubleClick);

    // Формируем строку статуса последнего запомненного состояния реле на основе считанного из памяти значения
    print_log("[SYSTEM] ", relayIsOn ? "Started: Relay is ON" : "Started: Relay is OFF");
    display_print_status("[SYSTEM] ", relayIsOn ? "Relay is ON" : "Relay is OFF");
    if(relayIsOn) WriteColorPixel(COLORS_RGB_LED::green);
    else WriteColorPixel(COLORS_RGB_LED::red);
    print_log("[SYSTEM] ", "TX Ready: 1 click = ON, 2 clicks = OFF");
  #endif

  

  // 7. Финальные действия для RX
  #ifdef RECEIVER
    #if defined(ARDUINO_ARCH_ESP8266)
      EEPROM.begin(4); // Инициализируем 4 байта (нам нужен только 0-й адрес)
      relayIsOn = (EEPROM.read(0) == 1); // Читаем байт: 1 = ON, 0 = OFF
      // Устанавливаем реле в сохраненное состояние , которое было сохранено в память при последнем выключении питания
      digitalWrite(RELAY_PIN, relayIsOn ? LOW : HIGH);
    #endif
    // log_radio_event(0, "RX Listening...");
    print_log("[SYSTEM] ", "RX Ready: Waiting for commands...");
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

        if (rxMessage == CMD_RELAY_ON)
        {
          digitalWrite(RELAY_PIN, LOW);                   // ВКЛЮЧАЕМ реле постоянно
          relayIsOn = true; // Обновляем локальную переменную
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 1); // Записываем "включено"
            EEPROM.commit();    // Фиксируем во Flash
          #endif
          delay(TIMEOUT_WAITING_TX);                                      // ДАЕМ ПЕРЕДАТЧИКУ ВРЕМЯ ПЕРЕКЛЮЧИТЬСЯ НА ПРИЕМ
          MyRadio.send(ACK_FROM_RECEIVER_IF_ON);          // Подтверждаем включение
          display_print_status("RELAY", "STATUS: ON");
          //У ESP8266 нет RGB светодиода, поэтому этот участок только для ESP32
          #ifdef ARDUINO_ARCH_ESP32
            WriteColorPixel(COLORS_RGB_LED::green);
          #endif
        } 
        else if (rxMessage == CMD_RELAY_OFF) {
          digitalWrite(RELAY_PIN, HIGH);                  // ВЫКЛЮЧАЕМ реле
          relayIsOn = false; // Обновляем локальную переменную
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 0); // Записываем "выключено"
            EEPROM.commit();    // Фиксируем во Flash
          #endif
          delay(TIMEOUT_WAITING_TX);                                      // ДАЕМ ПЕРЕДАТЧИКУ ВРЕМЯ ПЕРЕКЛЮЧИТЬСЯ НА ПРИЕМ
          MyRadio.send(ACK_FROM_RECEIVER_IF_OFF);         // Подтверждаем выключение
          display_print_status("RELAY", "STATUS: OFF");
          //У ESP8266 нет RGB светодиода, поэтому этот участок только для ESP32
          #ifdef ARDUINO_ARCH_ESP32
            WriteColorPixel(COLORS_RGB_LED::black);
          #endif
        }


        // --- НОВАЯ ФИТЧА: Ответ на запрос статуса ---
        else if (rxMessage == CMD_GET_STATUS) {
           delay(50); // Короткая пауза перед ответом
           // Читаем реальное состояние пина (у ESP32 LOW это ВКЛ)
           if (digitalRead(RELAY_PIN) == LOW) {
             MyRadio.send(ACK_RELAY_IS_ON);
           } else {
             MyRadio.send(ACK_RELAY_IS_OFF);
           }
        }
        // --- Конец новой фитчи ---

        
        // Возвращаемся в режим прослушивания
        MyRadio.startListening();
        
      } else {
        print_log("RX", "Receive Error: " + String(state));
        MyRadio.startListening();
      }
    }
  #endif
}





// ЛОГИКА РАБОТЫ ПЕРЕДАТЧИКА (ТХ)
#ifdef TRANSMITTER
  
  /**
   * @brief - Функция обработки одиночного нажатия кнопки (Включение)
   * 
   * @param b - кнопка Button2
   */
  void handleTap(Button2& b)
  {
    if (isProcessing) return; // Игнорируем, если уже идет отправка
    print_log("[BUTTON] ", "One Click sends CMD_RELAY_ON");

    // Логика: если реле выключено, любое одиночное нажатие пытается его включить
    // if (!relayIsOn) {
      print_log("[ACTION] ", "One Click sends Requesting ON");
      if (sendCommandAndWaitAck(CMD_RELAY_ON)) {
        relayIsOn = true;
        #if defined(ARDUINO_ARCH_ESP32)
          pref.putBool("state", true); // Сохраняем во Flash
        #endif
        display_print_status("[INFO] ", "Relay is ON");
      // }
    } else {
      // Если уже включено, просто игнорируем или выводим инфо на экран
      print_log("[INFO] ", "Relay already ON. Use double click to turn OFF");
      display_print_status("[INFO] ", "Relay already is ON");
    }
}



  
  /**
   * @brief - Функция обработки двойного нажатия (Выключение)
   * 
   * @param b - кнопка Button2
   */
  void handleDoubleClick(Button2& b)
  {
    if (isProcessing) return; // Игнорируем, если уже идет отправка
    print_log("[BUTTON] ", "Double Click sends CMD_RELAY_OFF");
    
    // Логика: если реле включено, двойной клик выключает его
    if (relayIsOn) {
      print_log("[ACTION] ", "Double Click sends Requesting OFF");
      if (sendCommandAndWaitAck(CMD_RELAY_OFF)) {
        relayIsOn = false;
        #if defined(ARDUINO_ARCH_ESP32)
          pref.putBool("state", false); // Сохраняем во Flash
        #endif
        display_print_status("[INFO] ", "Relay is OFF");
      }
    } else {
      // Если уже выключено, просто игнорируем или выводим инфо на экран
      print_log("[INFO]", "Already OFF. Use single click to turn ON");
      display_print_status("[INFO] ", "Relay already is OFF");
    }
  }






  /**
   * @brief Функция отправки команды с ожиданием подтверждения 
   * 
   * @param cmd Команда для отправки 
   * @return true - успешное получение подтверждения 
   * @return false - не получено подтверждение
   */
  bool sendCommandAndWaitAck(String cmd) {
    isProcessing = true; // Блокируем новые срабатывания кнопки
    bool ackReceived = false;   // Флаг успешного получения подтверждения
    String response;            // Переменная для хранения ответа
    
    // 1. Отправляем команду приёмнику
    int state = MyRadio.send(cmd);
    if (state != RADIOLIB_ERR_NONE) //Если возникнет ошибка при отправке
    { 
      isProcessing = false; // Разблокируем кнопку
      print_log("[TX] ", "Error sending: " + String(state));
      return false;
    }

    // 2. Сразу включаем приемник, чтобы слушать ответ
    MyRadio.startListening();

    // 3. Ждем ответа строго определенное время (например, 1 секунды)
    print_log("[TX] ", "Waiting for ACK...");
    unsigned long startWait = millis();
    const unsigned long timeout = TIMEOUT_WAITING_RX; // Таймаут ожидания ответа в миллисекундах

    while (millis() - startWait < timeout)
    {
      // КРИТИЧЕСКИ ВАЖНО: продолжаем вызывать loop кнопки внутри цикла ожидания!
      // Это позволит библиотеке Button2 корректно завершить свои внутренние таймеры.
      //btn.loop();

      // ВАЖНО: продолжаем опрашивать радио в цикле
      if (MyRadio.isDataReady())
      {
        if (MyRadio.receive(response) == RADIOLIB_ERR_NONE)
        {
          // Проверяем, что пришел именно нужный нам ACK из settings.h
          if (response == (ACK_FROM_RECEIVER_IF_ON) || (response == ACK_FROM_RECEIVER_IF_OFF))
          {
            ackReceived = true;
            break; // Выходим из цикла, ответ получен!
          }
        }
      }
    // delay(10); // Чтобы ESP32 не "раскалился" в пустом цикле
    }

    // 4. Обрабатываем результат
    if (ackReceived)
    {
      WriteColorPixel(COLORS_RGB_LED::green);
      #ifdef VIBRO_USED
        digitalWrite(VIBRO_PIN, HIGH); delay(500); digitalWrite(VIBRO_PIN, LOW);
      #endif
      
      if(response == ACK_FROM_RECEIVER_IF_ON)
      {
        log_radio_event(0, "DONE! RX CONFIRMED ON");
      }
      else if(response == ACK_FROM_RECEIVER_IF_OFF)
      {
        log_radio_event(0, "DONE! RX CONFIRMED OFF");
      }

      display_print_status("[SUCCESS] ", "ACK received from RX");
    
    } else {
      WriteColorPixel(COLORS_RGB_LED::red);
      print_log("[TX] ", "TIMEOUT: Receiver not responding");
      display_print_status("[ERROR] ", "No ACK from RX");
    }
   
    // Возвращаем радио в режим прослушивания для следующего раза
    MyRadio.startListening();
    isProcessing = false; // Снимаем блокировку
    return ackReceived;
  }


#endif