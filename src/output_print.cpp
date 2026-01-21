#include <output_print.h>


/**
 * @brief Универсальная функция печати
 * 
 * @param status - строка статуса
 * @param message - строка сообщения
 */
void print_log(String status, String message) {
    #ifdef DEBUG_PRINT
        // Формат: [TX] SUCCESS : Сообщение
        Serial.print("[");
        Serial.print(RADIO_NAME);
        Serial.print("] ");
        Serial.print(status);
        Serial.print(" : ");
        Serial.println(message);
    #endif
}


// Реализация для константных строк (экономит память)
void print_log(const char* status, const char* message) {
    #ifdef DEBUG_PRINT
        print_log(String(status), String(message));
    #endif
}


/**
 * @brief 
 * 
 * @param state 
 * @param info 
 */
void print_radio_state(int state, String info) {
    #ifdef DEBUG_PRINT
        if (state == RADIOLIB_ERR_NONE) {
            print_log("SUCCESS", info.length() > 0 ? info : "Operation finished");
        } else {
            // Если ошибка, выводим код ошибки
            String errorMsg = "Code [" + String(state) + "] ";
            if (info.length() > 0) errorMsg += "(" + info + ")";
            print_log("ERROR", errorMsg);
        }
    #endif
}





/**
 * @brief Функция, выводит в сериал монитор сообщения
 * 
 * @param RadioName - тип радио: ТХ или РХ
 * @param string    - строка String для печати в терминал
 */
void print_string_to_serial(String &RadioName, String &state)
{
  String str = RadioName + " : " + state;
  Serial.println(str);
}



/**
 * @brief Функция, которая обеспечивает вывод текущего состояния 
 * приёмопередатчика в сериал-порт (если он задан)
 * 
 * @param state         - текущее состояние, полученное от передатчика при его работе
 * @param transmit_str  - строка для передачи
 */
void print_radio_state(int &state, String &transmit_str)
{
    String str;
    
    //Если передача успешна, выводим сообщение в сериал-монитор
    if (state == RADIOLIB_ERR_NONE) {
        //Выводим сообщение об успешной передаче
            str = F("TRANSMITT SUCCES!");
            print_string_to_serial(RADIO_NAME, str);
    } else {
        //Если были проблемы при передаче, сообщаем об этом
        str = (String)state;
        Serial.print(F("transmission failed, ERROR "));
        print_string_to_serial(RADIO_NAME, str);
    }
}


