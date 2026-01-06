#include <Arduino.h>
#include "Button2.h"
#include "settings.h"
#include "radiomodem.h"
#include "output_display.h"
#include "logger.h"
#include "rgb_led.h"



// ОПРЕДЕЛЯЕМ RADIO_NAME: берем из настроек, чтобы логгер знал, кто пишет (TX или RX)
#ifdef TRANSMITTER
  String RADIO_NAME = "TX";
#else
  String RADIO_NAME = "RX";
#endif



// Кнопка инициализируется только для передатчика
#ifdef TRANSMITTER
  Button2 btn(BUTTON_PIN);
  void handleTap(Button2& b); // Прототип функции обработки нажатия
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
    digitalWrite(RELAY_PIN, LOW);
  #endif

  // 3. Настройка светодиода (через библиотеку rgb_led.h)
  WriteColorPixel(COLORS_RGB_LED::blue); // В состоянии режима ожидания синий цвет

  // 4. Инициализация дисплея
  #ifdef USE_DISPLAY
    display_init();
  #endif

  // 5. Запуск радиомодема
  if (!MyRadio.beginRadio()) {
    log_radio_event(-1, "Radio Error!");
      while (1) { // Если радио не завелось — мигаем красным
        WriteColorPixel(COLORS_RGB_LED::red);
        delay(300);
        WriteColorPixel(COLORS_RGB_LED::black);
        delay(300);
      }
    }

    #ifdef TRANSMITTER
      btn.setTapHandler(handleTap);
      log_radio_event(0, "TX System Ready");
    #endif


    #ifdef RECEIVER
      log_radio_event(0, "RX Listening...");
      // Модем автоматически уходит в режим прослушки в MyRadio.beginRadio()
    #endif

    // Синий цвет — дежурный режим (система готова)
    WriteColorPixel(COLORS_RGB_LED::blue);
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
      String incoming = "";
      // Считываем данные из чипа
      int state = MyRadio.receive(incoming);

      if (state == RADIOLIB_ERR_NONE) 
      {
        log_radio_event(RADIOLIB_ERR_NONE, "GOT: " + incoming);
            
        // Если пришла нужная команда
        if (incoming == COMMAND_EN_RELAY) 
        {
          delay(50); // ДАЕМ ПЕРЕДАТЧИКУ ВРЕМЯ ПЕРЕКЛЮЧИТЬСЯ НА ПРИЕМ
          // ОТПРАВКА ОТВЕТА: Создаем переменную, чтобы VS Code не ругался на ссылку
          String ackMsg = String(ACK_FROM_RECEIVER);
          MyRadio.send(ackMsg);
                
          // 2. Проверяем, не выполняли ли мы это действие только что
          static unsigned long lastRelayTime = 0;
          // Если прошло меньше 3 секунд с последнего раза — игнорируем действие
          if (millis() - lastRelayTime > 3000)
          {
            log_radio_event(0, "RELAY ACTIVATE!");
            lastRelayTime = millis(); // Запоминаем время
            // Активация исполнительного устройства
            #ifdef RELAY_USED
              digitalWrite(RELAY_PIN, HIGH);
              delay(500); // Держим реле включенным полсекунды
              digitalWrite(RELAY_PIN, LOW);
            #endif
          }else{
            log_radio_event(0, "Action ignored (too fast)");
          }
        }
        // ВАЖНО: Снова переводим радио в режим "Слушать эфир"
        MyRadio.startListening(); 
      }
    }
  #endif
}









// ЛОГИКА РАБОТЫ ПЕРЕДАТЧИКА (ТХ)
#ifdef TRANSMITTER
void handleTap(Button2& b)
{
  log_radio_event(0, "Button Pressed");
  bool success = false;
  String command = String(COMMAND_EN_RELAY);
  String feedback = "";

  // Пытаемся отправить команду до 3-х раз
  for (int attempt = 1; attempt <= 3; attempt++)
  {
    WriteColorPixel(COLORS_RGB_LED::red); // Красный при передаче
    log_radio_event(0, "TX Attempt " + String(attempt));

    // Отправляем сигнал
    int tx_state = MyRadio.send(command);

    if (tx_state == RADIOLIB_ERR_NONE)
    {
      // Включаем прием сразу после отправки, чтобы поймать ответ
      MyRadio.startListening();

      log_radio_event(0, "Wait ACK...");
      
      // Ожидаем ответа от приемника (таймаут задается в settings.h)
      unsigned long startWait = millis();
      while (millis() - startWait < WAITING_RESPONCE)
      {
        if (MyRadio.isDataReady()) // Если пришел пакет
        {
          int rx_state = MyRadio.receive(feedback);
          if (rx_state == RADIOLIB_ERR_NONE && feedback == String(ACK_FROM_RECEIVER))
          {
            success = true;
            break; // Успех, выходим из ожидания
          }
        }
        yield(); // Чтобы ESP32 не ушла в перезагрузку по WDT
      }
    }

    if (success) break; // Если получили подтверждение, следующие попытки не нужны
    delay(100); // Пауза между попытками
  }
    
  // Результат операции
  if (success)
  {
    log_radio_event(0, "DONE! RX CONFIRMED");
    WriteColorPixel(COLORS_RGB_LED::green); // Зеленый — успех
    #ifdef VIBRO_USED
      digitalWrite(VIBRO_PIN, HIGH);
      delay(1000);
      digitalWrite(VIBRO_PIN, LOW);
    #endif
  }
  else
  {
    log_radio_event(-1, "FAIL: NO RESPONSE");
    WriteColorPixel(COLORS_RGB_LED::red); // Красный — ошибка связи
  }

  // Возвращаемся в дежурный режим через секунду
  delay(2000);
  WriteColorPixel(COLORS_RGB_LED::blue);
  
  // Возвращаем радио в режим приема
  MyRadio.startListening();
}
#endif