#pragma once

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "spi_ili9341.h"

#define DISPLAY_TAG "Display"
#define MAX_LINES 19
#define MAX_CHARS_PER_LINE 42
#define LEFT_MARGIN 13
#define TOP_MARGIN 6
#define LINE_HEIGHT 12
#define INDENT_WIDTH 7
#define PARAGRAPH_SPACING 0     // Дополнительный интервал между абзацами (в строках)
#define PARAGRAPH_DELIMITER '|' // Символ разделителя абзацев

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
    uint8_t current_line; // Текущая строка для вывода
} display_state_t;

esp_err_t display_init(const display_config_t *config, display_state_t *out_state);
void display_draw_text(display_state_t *state, const char *text, uint16_t x, uint16_t y);
void display_clear(display_state_t *state, uint16_t color);
void display_print_wrapped(display_state_t *state, const char *text);
void display_reset_cursor(display_state_t *state);
void display_print_wrapped_rus(display_state_t *state, const char *text);
char *get_build_date(void);
char *combine_strings(const char *str1, const char *str2);