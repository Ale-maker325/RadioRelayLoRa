#include <Arduino.h>
#include "Button2.h"        // Библиотека для удобной работы с кнопкой
#include "settings.h"       // Твои настройки (пины, частоты, команды)
#include "radiomodem.h"     // Логика работы LoRa модема
#include "output_display.h" // Функции для вывода текста на экран
#include "logger.h"         // Логирование событий
#include "rgb_led.h"        // Управление встроенным светодиодом

// --- Работа с памятью (чтобы состояние реле не сбрасывалось при выключении питания) ---
#if defined(ARDUINO_ARCH_ESP32)
  #include <Preferences.h>  // Библиотека для ESP32 (постоянная память NVS)
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <EEPROM.h>       // Старая добрая EEPROM для ESP8266
#endif

#ifdef TRANSMITTER
  // --- НАСТРОЙКИ ДЛЯ ПЕРЕДАТЧИКА (ПУЛЬТА) ---
  String RADIO_NAME = "TX";
  // Инициализируем кнопку: пин из настроек, подтяжка к питанию, активный сигнал - LOW (земля)
  Button2 btn(BUTTON_PIN, INPUT_PULLUP, true);
  
  #if defined(ARDUINO_ARCH_ESP32)
    Preferences pref; // Объект для работы с памятью ESP32
  #endif

  bool isProcessing = false; // Блокировщик: не дает отправить новую команду, пока ждем ответ на старую
  bool relayIsOn = false;    // Мы «думаем», что реле сейчас в этом состоянии
  bool rxOnline = false;     // Флаг: на связи ли приемник прямо сейчас

  // Объявление функций (прототипы), чтобы компилятор знал о них заранее
  void handleTap(Button2& b); 
  void handleDoubleClick(Button2& b); 
  bool sendCommandAndWaitAck(String cmd); 
  void updateDisplayStatus(String status, String msg); 
#else
  // --- НАСТРОЙКИ ДЛЯ ПРИЕМНИКА (ИСПОЛНИТЕЛЯ) ---
  String RADIO_NAME = "RX";
  bool relayIsOn = false; // Актуальное состояние реле на стороне приемника
#endif

// --- ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ ДЛЯ ПЕРЕДАТЧИКА ---
#ifdef TRANSMITTER
/**
 * Выводит статус на экран и меняет цвет светодиода.
 * Добавляет [OK], если связь есть, или [LOST], если связь потеряна.
 */
void updateDisplayStatus(String status, String msg) {
    String conn = rxOnline ? " [OK]" : " [LOST]";
    display_print_status(status + conn, msg);
    
    if (!rxOnline) WriteColorPixel(COLORS_RGB_LED::blue);     // Синий — нет связи
    else if (relayIsOn) WriteColorPixel(COLORS_RGB_LED::green); // Зеленый — реле включено
    else WriteColorPixel(COLORS_RGB_LED::red);                 // Красный — реле выключено
}
#endif

void setup()
{
  // Инициализация последовательного порта для отладки
  #ifdef DEBUG_PRINT
    Serial.begin(115200);
  #endif

  // 1. Короткая вибрация при включении (если настроено в settings.h)
  #if defined(TRANSMITTER) && defined(VIBRO_USED)
    pinMode(VIBRO_PIN, OUTPUT);
    digitalWrite(VIBRO_PIN, HIGH); delay(100); digitalWrite(VIBRO_PIN, LOW);
  #endif

  // 2. Настройка пина реле для Приемника
  #if defined(RECEIVER) && defined(RELAY_USED)
    pinMode(RELAY_PIN, OUTPUT);
    #if defined(ARDUINO_ARCH_ESP32)
      digitalWrite(RELAY_PIN, LOW); // У ESP32 логика может отличаться
    #elif defined(ARDUINO_ARCH_ESP8266)
      digitalWrite(RELAY_PIN, HIGH); // У ESP8266 обычно HIGH — это выкл. реле
    #endif
  #endif
  
  // 3. Инициализация дисплея и светодиода для ESP32
  #if defined(ARDUINO_ARCH_ESP32)
    WriteColorPixel(COLORS_RGB_LED::blue); // Зажигаем синим при старте
    #ifdef USE_DISPLAY
      display_init();
    #endif
  #endif

  // 4. Запуск радиомодуля LoRa
  if (!MyRadio.beginRadio()) {
    while (1) { // Если радио не инициализировалось — мигаем красным бесконечно
      WriteColorPixel(COLORS_RGB_LED::red);
      display_print_status("ERROR", "Radio Fail");
      delay(500);
    }
  }

  // 5. Специфичные настройки для Передатчика (чтение памяти и синхронизация)
  #ifdef TRANSMITTER
    #if defined(ARDUINO_ARCH_ESP32)
      pref.begin("relay-app", false); // Открываем хранилище "relay-app"
      relayIsOn = pref.getBool("state", false); // Достаем последнее сохраненное состояние
    #endif

    // Попытка узнать реальное состояние приемника сразу при включении пульта
    #ifdef RELAY_GET_STATUS
      print_log(RADIO_NAME, "Syncing...");
      rxOnline = sendCommandAndWaitAck(CMD_GET_STATUS); // Спрашиваем статус у приемника
    #endif
    
    // Назначаем функции на клики кнопки
    btn.setTapHandler(handleTap);           // Одиночный клик
    btn.setDoubleClickTime(700);            // Окно ожидания второго клика (мс)
    btn.setDoubleClickHandler(handleDoubleClick); // Двойной клик

    // Выводим информацию на экран после загрузки
    updateDisplayStatus("SYSTEM", relayIsOn ? "Relay: ON" : "Relay: OFF");
  #endif

  // 6. Специфичные настройки для Приемника (чтение памяти EEPROM)
  #ifdef RECEIVER
    #if defined(ARDUINO_ARCH_ESP8266)
      EEPROM.begin(4); // Инициализация 4 байт памяти
      relayIsOn = (EEPROM.read(0) == 1); // Читаем сохраненное состояние
      digitalWrite(RELAY_PIN, relayIsOn ? LOW : HIGH); // Сразу выставляем реле
    #endif
    print_log("[SYSTEM] ", "RX Ready...");
  #endif
}

void loop()
{
  #ifdef TRANSMITTER
    btn.loop(); // Постоянно опрашиваем кнопку на предмет нажатий
  #endif

  #ifdef RECEIVER
    // ПРИЕМНИК: проверяем, пришло ли что-то по радио
    if (MyRadio.isDataReady()) {
      String rxMessage;
      if (MyRadio.receive(rxMessage) == RADIOLIB_ERR_NONE) { // Если данные приняты без ошибок
        
        if (rxMessage == CMD_RELAY_ON) {
          // Команда на включение
          digitalWrite(RELAY_PIN, LOW); relayIsOn = true;
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 1); EEPROM.commit(); // Сохраняем в память
          #endif
          delay(TIMEOUT_WAITING_TX); // Даем время передатчику переключиться на прием
          MyRadio.send(ACK_FROM_RECEIVER_IF_ON); // Отправляем подтверждение "Я включился"
          display_print_status("RELAY", "STATUS: ON");
          
        } else if (rxMessage == CMD_RELAY_OFF) {
          // Команда на выключение
          digitalWrite(RELAY_PIN, HIGH); relayIsOn = false;
          #if defined(ARDUINO_ARCH_ESP8266)
            EEPROM.write(0, 0); EEPROM.commit();
          #endif
          delay(TIMEOUT_WAITING_TX);
          MyRadio.send(ACK_FROM_RECEIVER_IF_OFF); // Подтверждение "Я выключился"
          display_print_status("RELAY", "STATUS: OFF");
          
        } else if (rxMessage == CMD_GET_STATUS) {
          // Запрос статуса от передатчика
          delay(50);
          // Отвечаем текущим состоянием пина реле
          MyRadio.send((digitalRead(RELAY_PIN) == LOW) ? ACK_RELAY_IS_ON : ACK_RELAY_IS_OFF);
        }
        
        // После обработки возвращаемся в режим прослушивания эфира
        MyRadio.startListening();
      }
    }
  #endif
}

#ifdef TRANSMITTER
// --- ОБРАБОТКА НАЖАТИЙ (Только для пульта) ---

/**
 * Одиночное нажатие — ВКЛЮЧИТЬ
 */
void handleTap(Button2& b) {
    if (isProcessing) return; // Если уже что-то отправляем, игнорируем нажатие
    
    // Если мы видим, что реле выключено ИЛИ связь была потеряна — пытаемся включить
    if (!relayIsOn || !rxOnline) {
        print_log("[ACTION]", "Sending ON command...");
        if (sendCommandAndWaitAck(CMD_RELAY_ON)) {
            relayIsOn = true;
            #if defined(ARDUINO_ARCH_ESP32)
              pref.putBool("state", true); // Запоминаем успех в память
            #endif
            updateDisplayStatus("SUCCESS", "Relay is ON");
        } else {
            // Если подтверждение не пришло (таймаут)
            updateDisplayStatus("ERROR", "No RX Link");
        }
    } else {
        updateDisplayStatus("INFO", "Already ON"); // Реле и так включено, ничего не делаем
    }
}

/**
 * Двойное нажатие — ВЫКЛЮЧИТЬ
 */
void handleDoubleClick(Button2& b) {
    if (isProcessing) return;

    // Если реле включено ИЛИ связи нет — пробуем выключить
    if (relayIsOn || !rxOnline) {
        print_log("[ACTION]", "Sending OFF command...");
        if (sendCommandAndWaitAck(CMD_RELAY_OFF)) {
            relayIsOn = false;
            #if defined(ARDUINO_ARCH_ESP32)
              pref.putBool("state", false);
            #endif
            updateDisplayStatus("SUCCESS", "Relay is OFF");
        } else {
            updateDisplayStatus("ERROR", "No RX Link");
        }
    } else {
        updateDisplayStatus("INFO", "Already OFF");
    }
}

/**
 * Ключевая функция: отправить команду и ждать подтверждения (ACK)
 */
bool sendCommandAndWaitAck(String cmd) {
    isProcessing = true; // Занимаем канал
    bool ackReceived = false;
    String response;

    MyRadio.send(cmd);         // Посылаем команду в эфир
    MyRadio.startListening();  // Сразу переходим в режим приема ответа

    unsigned long startWait = millis(); // Засекаем время начала ожидания
    // Цикл ожидания ответа (длится до наступления таймаута)
    while (millis() - startWait < TIMEOUT_WAITING_RX) {
        btn.loop(); // Важно: продолжаем опрашивать кнопку, чтобы она не «глючила» после цикла
        
        if (MyRadio.isDataReady()) {
            if (MyRadio.receive(response) == RADIOLIB_ERR_NONE) {
                // Если пришел ответ, проверяем — подтверждает ли он включение...
                if (response == ACK_FROM_RECEIVER_IF_ON || response == ACK_RELAY_IS_ON) {
                    relayIsOn = true; ackReceived = true; break;
                }
                // ... или выключение
                if (response == ACK_FROM_RECEIVER_IF_OFF || response == ACK_RELAY_IS_OFF) {
                    relayIsOn = false; ackReceived = true; break;
                }
            }
        }
    }

    rxOnline = ackReceived; // Если получили ответ — значит приемник «в сети»
    isProcessing = false;   // Освобождаем кнопку для новых команд
    return ackReceived;     // Возвращаем результат: успешно (true) или нет (false)
}
#endif