#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "spi_ili9341.h"

#define num_row(i) i * 12

// Конфиг пинов (теперь не хардкод)
typedef struct
{
    int clk;
    int mosi;
    int dc;
    int rst;
    int cs;
} DisplayPins_t;

// Лог-тэг
static const char *TAG = "Display";

// Инициализация дисплея
void display_init(const DisplayPins_t *pins, spi_device_handle_t *spi_out);