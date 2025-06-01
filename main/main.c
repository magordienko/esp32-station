#include "main.h"

static int display_dc_pin = -1; // Храним DC-пин здесь

static void spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc_level = (int)t->user;
    gpio_set_level(display_dc_pin, dc_level);
}

esp_err_t display_init(const display_config_t *config, display_state_t *out_state)
{
    esp_err_t ret;
    display_dc_pin = config->pins.dc; // Сохраняем DC-пин

    // Настройка GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->pins.dc) | (1ULL << config->pins.rst),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Инициализация SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = config->pins.mosi,
        .miso_io_num = -1,
        .sclk_io_num = config->pins.clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * config->width * 2 + 8,
    };

    ret = spi_bus_initialize(config->spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
        return ret;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 32 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = config->pins.cs,
        .queue_size = 7,
        .pre_cb = spi_pre_transfer_callback,
    };

    ret = spi_bus_add_device(config->spi_host, &dev_cfg, &out_state->spi);
    if (ret != ESP_OK)
        return ret;

    // Инициализация дисплея
    TFT9341_ini(out_state->spi, config->width, config->height);
    TFT9341_FillScreen(out_state->spi, TFT9341_BLACK);

    // Настройки по умолчанию
    out_state->text_color = TFT9341_GREEN;
    out_state->bg_color = TFT9341_BLACK;
    out_state->font = &Font12;

    return ESP_OK;
}

void display_draw_text(display_state_t *state, const char *text, uint16_t x, uint16_t y)
{
    TFT9341_SetTextColor(state->text_color);
    TFT9341_SetBackColor(state->bg_color);
    TFT9341_SetFont(state->font);
    TFT9341_String(state->spi, x, y, (char *)text);
}

void display_clear(display_state_t *state, uint16_t color)
{
    TFT9341_FillScreen(state->spi, color);
}

// Пример использования
void app_main()
{
    display_config_t config = {
        .spi_host = HSPI_HOST,
        .pins = {.clk = 18, .mosi = 23, .dc = 27, .rst = 26, .cs = 25},
        .width = 320,
        .height = 240};

    display_state_t display;
    if (display_init(&config, &display) != ESP_OK)
    {
        ESP_LOGE(DISPLAY_TAG, "Display initialization failed!");
        return;
    }

    TFT9341_SetRotation(display.spi, 3);

    const char *lines[] = {
        "Project: EPS32-Station.",
        "Date: 01/06/2025 year.",
        "",
        "Line 4",
        "Line 5",
        "Line 6",
        "Line 7",
        "Line 8",
        "Line 9",
        "Line 10",
        "Line 11",
        "Line 12",
        "Line 13",
        "Line 14",
        "Line 15",
        "Line 16",
        "Line 17",
        "Line 18",
        "Line 19: Midnight frost: owls call, wind s"};

    while (1)
    {
        for (uint8_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
        {
            uint16_t x = 13;
            uint16_t y = 6 + i * 12;
            display_draw_text(&display, lines[i], x, y);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
        display_clear(&display, TFT9341_BLACK);
    }
}