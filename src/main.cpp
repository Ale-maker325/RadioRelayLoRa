#include <Arduino.h>
#include "Button2.h"        // Позволяет легко обрабатывать клики и двойные клики без написания сложных счетчиков времени
#include "settings.h"       // Тут лежат названия твоих команд (типа "ON", "OFF") и номера пинов
#include "radiomodem.h"     // Твой "пульт управления" радиочипом (отправка/прием данных через LoRa)
#include "output_display.h" // Функции для рисования текста на маленьком экране
#include "logger.h"         // Помогает выводить красивые сообщения в монитор порта на компьютере
#include "rgb_led.h"        // Управляет цветом маленького светодиода на самой плате

/** * РАЗБОР РАБОТЫ С ЭНЕРГОНЕЗАВИСИМОЙ ПАМЯТЬЮ (NVS и EEPROM):
 * * Нам нужно, чтобы после выключения батарейки пульт помнил, включен свет или нет.
 * * --- ДЛЯ ESP32 (используется Preferences) ---
 * 1. pref.begin("relay-app", false) — Мы открываем в памяти "хранилище" с именем "relay-app". 
 * Параметр 'false' означает, что мы собираемся туда и писать, и читать (не "только чтение").
 * 2. pref.getBool("state", false) — Мы просим выдать нам значение под ключом "state". 
 * Если это самый первый запуск и там ничего нет, функция вернет 'false' (значение по умолчанию).
 * 3. pref.putBool("state", true/false) — Мы записываем новое состояние. Это происходит мгновенно 
 * и сразу сохраняется в чип памяти. Это надежнее старой EEPROM.
 * * --- ДЛЯ ESP8266 (используется EEPROM) ---
 * 1. EEPROM.begin(4) — Мы говорим: "Выдели нам 4 байта памяти". Это подготовка буфера в оперативной памяти.
 * 2. EEPROM.read(0) — Мы читаем байт из ячейки №0. Память тут представлена как длинная строка ячеек, 
 * у которых есть только номера, а не имена.
 * 3. EEPROM.write(0, значение) — Мы записываем данные в ячейку. Но внимание: данные еще НЕ сохранились в чип!
 * 4. EEPROM.commit() — САМЫЙ ВАЖНЫЙ ШАГ. Только после этой команды данные физически переносятся из 
 * оперативной памяти в постоянную. Если забыть commit, после перезагрузки всё пропадет.
 */

#if defined(ARDUINO_ARCH_ESP32)
  #include <Preferences.h>  // Библиотека для работы с NVS-памятью ESP32
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <EEPROM.h>       // Стандартный способ сохранения данных для старых плат
#endif

#ifdef TRANSMITTER
  // --- НАСТРОЙКИ ДЛЯ ПЕРЕДАТЧИКА (ПУЛЬТА) ---
  String RADIO_NAME = "TX";
  
  // Создаем объект кнопки. BUTTON_PIN — из settings.h, INPUT_PULLUP — подтягивает пин к питанию,
  // чтобы он не "болтался в воздухе", true — кнопка замыкается на землю.
  Button2 btn(BUTTON_PIN, INPUT_PULLUP, true);
  
  #if defined(ARDUINO_ARCH_ESP32)
    Preferences pref; // Создаем инструмент для работы с памятью
  #endif

  // Прототипы функций (просто оглавление для компилятора)
  //void handleTap(Button2& b); 
  void handleClick(Button2& b);
  void handleDoubleClick(Button2& b); 
  bool sendCommandAndWaitAck(String cmd);
  // int numClicks = 0;
  void updateDisplayStatus(String status, String msg); 
#else
  // --- НАСТРОЙКИ ДЛЯ ПРИЕМНИКА (ИСПОЛНИТЕЛЯ) ---
  String RADIO_NAME = "RX";
  bool relayIsOn = false; // Фактическое состояние пина реле
#endif

#ifdef TRANSMITTER
/**
 * Функция, которая "рисует" статус на экране и меняет цвет светодиода.
 * Нужна для того, чтобы не писать одни и те же команды по 10 раз в разных местах.
 */
void updateDisplayStatus(String status, String msg) {
    // Формируем строчку связи: [OK] если связь есть, или [LOST] если нет
    String conn = MyRadio.rxOnline ? " [RELAY ONLINE]" : " [RELAY NOT ONLINE]";
    display_print_status(status + conn, msg);
    
    // Меняем цвет встроенного RGB светодиода:
    if (!MyRadio.rxOnline) WriteColorPixel(COLORS_RGB_LED::blue);       // СИНИЙ — нет связи (или еще не проверяли)
    else if (MyRadio.relayIsOn) WriteColorPixel(COLORS_RGB_LED::green); // ЗЕЛЕНЫЙ — всё включено, связь есть
    else WriteColorPixel(COLORS_RGB_LED::red);                 // КРАСНЫЙ — всё выключено, связь есть
}
#endif










void setup()
{
  // Включаем передачу данных в компьютер для отладки
  #ifdef DEBUG_PRINT
    Serial.begin(115200);
  #endif

  // 1. Делаем короткий "вжжжух" вибромоторчиком при включении (если он есть в схеме)
  #if defined(TRANSMITTER) && defined(VIBRO_USED)
    pinMode(VIBRO_PIN, OUTPUT);
    digitalWrite(VIBRO_PIN, HIGH); delay(100); digitalWrite(VIBRO_PIN, LOW);
  #endif

  // 2. Настраиваем ножку (пин), которая дергает реле
  #if defined(RECEIVER) && defined(RELAY_USED)
    pinMode(RELAY_PIN, OUTPUT);
    #if defined(ARDUINO_ARCH_ESP32)
      digitalWrite(RELAY_PIN, LOW); // Для ESP32 по умолчанию ставим 0
    #elif defined(ARDUINO_ARCH_ESP8266)
      digitalWrite(RELAY_PIN, HIGH); // Для ESP8266 обычно реле выключено при 1 (HIGH)
    #endif
  #endif
  
  // 3. Запускаем экран и красим светодиод в синий (значит "Гружусь...")
  #if defined(ARDUINO_ARCH_ESP32)
    WriteColorPixel(COLORS_RGB_LED::blue); 
    #ifdef USE_DISPLAY
      display_init(); // Запуск экрана
    #endif
  #endif

  // 4. Проверяем радиомодуль. Если он не подключен — мигаем КРАСНЫМ и дальше не идем
  if (!MyRadio.beginRadio()) {
    while (1) {
      WriteColorPixel(COLORS_RGB_LED::red);
      display_print_status("ERROR", "Radio Fail");
      delay(500);
    }
  }

  // 5. Особые действия для ПУЛЬТА при включении
  #ifdef TRANSMITTER
    #if defined(ARDUINO_ARCH_ESP32)
      // Вспоминаем, что было до выключения питания
      pref.begin("relay-app", false); 
      MyRadio.relayIsOn = pref.getBool("state", false); 
    #endif

    // Если в настройках включен опрос статуса — спрашиваем у приемника, как он там
    #ifdef RELAY_GET_STATUS
      print_log(RADIO_NAME, "Syncing...");
      MyRadio.rxOnline = MyRadio.sendCommandAndWaitAck(CMD_GET_STATUS, [](){ btn.loop(); }); // Функция сама вернет true или false
    #endif
    
    // Настраиваем кнопку:
    // btn.setTapHandler(handleTap);           // Один клик — ВКЛЮЧИТЬ
    btn.setClickHandler(handleClick);         // Один клик — ВКЛЮЧИТЬ
    btn.setLongClickHandler(handleClick); // Длинное нажатие тоже включит реле
    btn.setDoubleClickTime(700);            // Время (мс), за которое нужно успеть нажать второй раз
    btn.setDoubleClickHandler(handleDoubleClick); // Два клика — ВЫКЛЮЧИТЬ

    // Обновляем экран: показываем, что мы вспомнили из памяти
    updateDisplayStatus(RADIO_NAME, MyRadio.relayIsOn ? "Last relay was ON" : "Last relay was OFF");
    print_log(RADIO_NAME, MyRadio.relayIsOn ? "Last set relay was ON" : "Last set relay was OFF");
  #endif

  // 6. Особые действия для ПРИЕМНИКА при включении
  #ifdef RECEIVER
    #if defined(ARDUINO_ARCH_ESP8266)
      EEPROM.begin(4); // Готовим память
      MyRadio.relayIsOn = (EEPROM.read(0) == 1); // Читаем состояние
      digitalWrite(RELAY_PIN, MyRadio.relayIsOn ? LOW : HIGH); // Сразу ставим реле как было
    #endif
    print_log("[SYSTEM] ", "RX Ready...");
  #endif
}

void loop()
{
  #ifdef TRANSMITTER
    btn.loop(); // ОЧЕНЬ ВАЖНО: постоянно проверяем кнопку, иначе она не сработает
  #endif

  #ifdef RECEIVER
    // Секция приема: слушаем эфир, не летит ли нам команда
    if (MyRadio.isDataReady()) {
      String rxMessage;
      // Если данные получены без помех:
      if (MyRadio.receive(rxMessage) == RADIOLIB_ERR_NONE) { 
        
        if (rxMessage == CMD_RELAY_ON) {
          digitalWrite(RELAY_PIN, LOW); relayIsOn = true; // ВКЛ
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 1); EEPROM.commit(); // Запомнили в память
          #endif
          delay(TIMEOUT_WAITING_TX); // Ждем чуть-чуть, пока пульт перейдет в режим приема подтверждения
          MyRadio.send(ACK_FROM_RECEIVER_IF_ON); // Отвечаем "Я всё сделал!"
          display_print_status("RELAY", "STATUS: ON");
          
        } else if (rxMessage == CMD_RELAY_OFF) {
          digitalWrite(RELAY_PIN, HIGH); relayIsOn = false; // ВЫКЛ
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 0); EEPROM.commit();
          #endif
          delay(TIMEOUT_WAITING_TX);
          MyRadio.send(ACK_FROM_RECEIVER_IF_OFF); // Отвечаем "Я всё сделал!"
          display_print_status("RELAY", "STATUS: OFF");
          
        } else if (rxMessage == CMD_GET_STATUS) {
          // Если нас просто спросили "Ты как?", отвечаем текущим состоянием ножки реле
          delay(50);
          MyRadio.send((digitalRead(RELAY_PIN) == LOW) ? ACK_RELAY_IS_ON : ACK_RELAY_IS_OFF);
        }
        
        MyRadio.startListening(); // Снова переходим в режим ожидания команд
      }
    }
  #endif
}




















#ifdef TRANSMITTER
/**
 * Обработка одного клика:
 * Мы хотим включить реле.
 */
void handleClick(Button2& b) {
    if (MyRadio.isProcessing) return; // Если уже идет какой-то обмен данными — игнорируем лишние нажатия

    
    
    // Условие: если мы ДУМАЕМ, что реле выключено, ИЛИ если у нас нет связи (надо проверить)
    if (!MyRadio.relayIsOn || !MyRadio.rxOnline) {
        print_log("[ACTION]", "Sending ON command...");
        // Пытаемся отправить команду и ждем ответ
        if (MyRadio.sendCommandAndWaitAck(CMD_RELAY_ON, [](){ btn.loop(); })) {
            MyRadio.relayIsOn = true; // Если ответ пришел (true) — значит реле точно ВКЛ
            #if defined(ARDUINO_ARCH_ESP32)
              pref.putBool("state", true); // Сохраняем успех в память
            #endif
            updateDisplayStatus(RADIO_NAME, "RX ON");
            print_log("[handleTap] :", "RX is ON");
        } else {
            // Если за 2 секунды никто не ответил
            updateDisplayStatus("[ERR]", "RX NOT ANSWER");
            print_log("[handleTap] :", "No answer from RX");
        }
    } else {
      updateDisplayStatus("[INFO]", "RX ALREADY ON");
      print_log("[handleTap] :", "RX already ON");
    }
}





/**
 * Обработка двойного клика:
 * Мы хотим выключить реле.
 */
void handleDoubleClick(Button2& b) {
    if (MyRadio.isProcessing) return;

    if (MyRadio.relayIsOn || !MyRadio.rxOnline) {
        print_log("[ACTION]", "Sending OFF command...");
        if (MyRadio.sendCommandAndWaitAck(CMD_RELAY_OFF, [](){ btn.loop(); })) {
            MyRadio.relayIsOn = false; // Успешно выключили
            #if defined(ARDUINO_ARCH_ESP32)
              pref.putBool("state", false); // Сохранили в память
            #endif
            updateDisplayStatus(RADIO_NAME, "RX OFF");
            print_log("[handleDoubleClick] :", "RX is OFF");
        } else {
            updateDisplayStatus("[ERR]", "RX NOT ANSWER");
            print_log("[handleDoubleClick] :", "No answer from RX");
        }
    } else {
        updateDisplayStatus("[INFO]", "RX ALREADY OFF");
        print_log("[handleDoubleClick] :", "RX already OFF");
    }
}



#endif