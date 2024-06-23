#include "main.h"
//------------------------------------------------
static const char *TAG = "main";

extern uint16_t TFT9341_WIDTH;
extern uint16_t TFT9341_HEIGHT;
//------------------------------------------------
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}
//------------------------------------------------
void app_main(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;
    // Configure SPI bus
    spi_bus_config_t cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * 320 * 2 + 8,
    };
    ret = spi_bus_initialize(HSPI_HOST, &cfg, SPI_DMA_CH_AUTO);
    ESP_LOGI(TAG, "spi bus initialize: %d", ret);
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 32000000,              // Clock out at 10 MHz
        .mode = 0,                               // SPI mode 0
        .spics_io_num = PIN_NUM_CS,              // CS pin
        .queue_size = 7,                         // We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
    };
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_LOGI(TAG, "spi bus add device: %d", ret);
    TFT9341_ini(spi, 320, 240);
    TFT9341_FillScreen(spi, TFT9341_WHITE);
    TFT9341_FillScreen(spi, TFT9341_BLACK);
    while (1)
    {
        // TFT9341_FillScreen(spi, TFT9341_BLACK);
        TFT9341_SetRotation(spi, 3);
        TFT9341_SetTextColor(TFT9341_GREEN);
        TFT9341_SetBackColor(TFT9341_BLACK);
        TFT9341_SetFont(&Font12);

        vTaskDelay(18000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 0, num_row(0), "I'm giving you a nightcall to tell you");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 140, num_row(1), "how I feel.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 0, num_row(3), "I want to drive you through the night,");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 140, num_row(4), "down the hills.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 0, num_row(6), "I'm gonna tell you something you");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 140, num_row(7), "don't want to hear,");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 0, num_row(9), "I'm gonna show you where its dark,");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        TFT9341_String(spi, 140, num_row(10), "but have no fear.");

        vTaskDelay(10000 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLACK);
    }
}