#pragma once
#include <Arduino.h>
#include <settings.h>

/**
 * @brief инициализация дисплея
 * 
 */
void display_init();

/**
 * @brief Вывод информации на дисплей
 * 
 * @param status - строка для вывода статуса
 * @param message - строка для вывода сообщения
 */
void display_print_status(String status, String message);

/**
 * @brief Очистка экрана
 * 
 */
void display_clear();
