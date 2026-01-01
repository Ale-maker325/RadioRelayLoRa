#pragma once
#include <Arduino.h>
#include "output_display.h"
#include "output_print.h"

/**
 * @brief Общая функция вывода на печать в сериал порт и на экран в зависимости от настроек конфигурации.
 * Внутри функция анализирует: если int state не равно нулю, значит неуспех и выводит на печать в порт номер ошибки
 * с указанием ERR:... а если статус равен нулю, то успех и выводится OK:
 * 
 * @param state - строка типа int, содержащая какой-то номер статуса
 * @param message - сопроводительная пояснительная строка
 */
void log_radio_event(int state, String message);