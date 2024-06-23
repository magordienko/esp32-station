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
    uint16_t i, j;
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
    while (1)
    {
        /*
        vTaskDelay(500 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLACK);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_RED);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLUE);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        for (i = 0; i < 20; i++)
        {
            TFT9341_FillRect(spi, 0, 0, TFT9341_WIDTH / 2 - 1, TFT9341_HEIGHT / 2 - 1, rand() & 0x0000FFFF);
            TFT9341_FillRect(spi, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH - 1, TFT9341_HEIGHT / 2 - 1, rand() & 0x0000FFFF);
            TFT9341_FillRect(spi, 0, TFT9341_HEIGHT / 2, TFT9341_WIDTH / 2 - 1, TFT9341_HEIGHT - 1, rand() & 0x0000FFFF);
            TFT9341_FillRect(spi, TFT9341_WIDTH / 2, TFT9341_HEIGHT / 2, TFT9341_WIDTH - 1, TFT9341_HEIGHT - 1, rand() & 0x0000FFFF);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLACK);

        for (i = 0; i < 300; i++)
        {
            TFT9341_FillRect(spi, rand() % TFT9341_WIDTH,
                             rand() % TFT9341_HEIGHT,
                             rand() % TFT9341_WIDTH,
                             rand() % TFT9341_HEIGHT,
                             rand() & 0x0000FFFF);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLACK);
        while (1)
        {
            for (j = 0; j < 500; j++)
            {
                TFT9341_DrawPixel(spi, rand() % TFT9341_WIDTH,
                                  rand() % TFT9341_HEIGHT,
                                  TFT9341_BLACK);
            }
            TFT9341_DrawPixel(spi, rand() % TFT9341_WIDTH,
                              rand() % TFT9341_HEIGHT,
                              TFT9341_WHITE);
            usleep(50);
        }
        */
        TFT9341_FillScreen(spi, TFT9341_BLACK);
        TFT9341_SetTextColor(TFT9341_YELLOW);
        TFT9341_SetBackColor(TFT9341_BLUE);
        TFT9341_SetFont(&Font24);
        TFT9341_DrawChar(spi, 10, 10, 'E');
        TFT9341_DrawChar(spi, 27, 10, 's');
        TFT9341_DrawChar(spi, 44, 10, 'p');
        TFT9341_DrawChar(spi, 61, 10, '3');
        TFT9341_DrawChar(spi, 78, 10, '2');
        TFT9341_SetTextColor(TFT9341_GREEN);
        TFT9341_SetBackColor(TFT9341_RED);
        TFT9341_SetFont(&Font20);
        TFT9341_DrawChar(spi, 10, 34, 'E');
        TFT9341_DrawChar(spi, 24, 34, 's');
        TFT9341_DrawChar(spi, 38, 34, 'p');
        TFT9341_DrawChar(spi, 52, 34, '3');
        TFT9341_DrawChar(spi, 66, 34, '2');
        TFT9341_SetTextColor(TFT9341_BLUE);
        TFT9341_SetBackColor(TFT9341_YELLOW);
        TFT9341_SetFont(&Font16);
        TFT9341_DrawChar(spi, 10, 54, 'E');
        TFT9341_DrawChar(spi, 21, 54, 's');
        TFT9341_DrawChar(spi, 32, 54, 'p');
        TFT9341_DrawChar(spi, 43, 54, '3');
        TFT9341_DrawChar(spi, 54, 54, '2');
        TFT9341_SetTextColor(TFT9341_CYAN);
        TFT9341_SetBackColor(TFT9341_BLACK);
        TFT9341_SetFont(&Font12);
        TFT9341_DrawChar(spi, 10, 70, 'E');
        TFT9341_DrawChar(spi, 17, 70, 's');
        TFT9341_DrawChar(spi, 24, 70, 'p');
        TFT9341_DrawChar(spi, 31, 70, '3');
        TFT9341_DrawChar(spi, 38, 70, '2');
        TFT9341_SetTextColor(TFT9341_RED);
        TFT9341_SetBackColor(TFT9341_GREEN);
        TFT9341_SetFont(&Font8);
        TFT9341_DrawChar(spi, 10, 82, 'E');
        TFT9341_DrawChar(spi, 15, 82, 's');
        TFT9341_DrawChar(spi, 20, 82, 'p');
        TFT9341_DrawChar(spi, 25, 82, '3');
        TFT9341_DrawChar(spi, 30, 82, '2');
        TFT9341_SetTextColor(TFT9341_YELLOW);
        TFT9341_SetBackColor(TFT9341_BLUE);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        TFT9341_FillScreen(spi, TFT9341_BLACK);
        for (i = 0; i < 4; i++)
        {
            TFT9341_SetRotation(spi, i % 4);
            TFT9341_SetFont(&Font24);
            TFT9341_FillScreen(spi, TFT9341_BLACK);
            TFT9341_String(spi, 1, 100, "ABCDEF12345678");
            TFT9341_SetFont(&Font20);
            TFT9341_String(spi, 1, 124, "ABCDEFGHI12345678");
            TFT9341_SetFont(&Font16);
            TFT9341_String(spi, 1, 144, "ABCDEFGHIKL123456789");
            TFT9341_SetFont(&Font12);
            TFT9341_String(spi, 1, 160, "ABCDEFGHIKLMNOPQRSTUVWXY 123456789");
            TFT9341_SetFont(&Font8);
            TFT9341_String(spi, 1, 172, "ABCDEFGHIKLMNOPQRSTUVWXYZ 123456789ABCDEFGHIKL");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        TFT9341_SetRotation(spi, 0);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}