#include "logger.h"
#include "settings.h"

/**
 * @brief Функция логгирования событий связанных с радио
 * 
 * @param state - состояние в числовом представлении
 * @param message - вывод сопутствующего сообщения
 */
void log_radio_event(int state, String message) {
    String status = (state == 0) ? "OK" : "ERR:" + String(state);

    #ifdef DEBUG_PRINT
      print_log(status, message); 
    #endif

    #ifdef USE_DISPLAY
      // Исправлено: имя функции должно совпадать с output_display.h
      display_print_status(status, message);
    #endif
}