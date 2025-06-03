#pragma once

#include <stdint.h>

// Структура описания шрифта
typedef struct
{
  const uint8_t *table; // Указатель на таблицу символов
  uint16_t Width;       // Ширина символа в пикселях
  uint16_t Height;      // Высота символа в пикселях
} sFONT;

// Внешнее объявление шрифта (определение в fonts.c)
extern const sFONT Font12;