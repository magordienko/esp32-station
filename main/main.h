#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "spi_ili9341.h"

#define DISPLAY_TAG "Display"

typedef struct
{
    spi_host_device_t spi_host;
    struct
    {
        int clk;
        int mosi;
        int dc;
        int rst;
        int cs;
    } pins;
    uint16_t width;
    uint16_t height;
} display_config_t;

typedef struct
{
    spi_device_handle_t spi;
    uint16_t text_color;
    uint16_t bg_color;
    sFONT *font;
} display_state_t;

esp_err_t display_init(const display_config_t *config, display_state_t *out_state);
void display_draw_text(display_state_t *state, const char *text, uint16_t x, uint16_t y);
void display_clear(display_state_t *state, uint16_t color);