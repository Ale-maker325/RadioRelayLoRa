#pragma once

#include <Arduino.h>
#include <settings.h>
#include <RadioLib.h>


/**
 * @brief Основная функция для печати
 * 
 * @param status - строка для вывода статуса или чего-нибудь ещё
 * @param message - строка сообщения для вывода
 */
void print_log(String status, String message);


/**
 * @brief Перегрузка функции print_log(String status, String message), чтобы можно было печатать просто текст в кавычках
 * 
 * @param status - строка для вывода статуса
 * @param message - строка для вывода сообщения
 */
void print_log(const char* status, const char* message);

// Твоя функция для состояния радио
/**
 * @brief Функция для печати о состоянии радиомодема
 * 
 * @param state - переменная, хранящая состояние модема
 * @param info - информационная строка о том, что произошло (или ещё что-нибудь на печать)
 */
void print_radio_state(int state, String info = "");