#include "main.h"

// Колбэк для SPI (управление DC пином)
static void spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc_level = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc_level);
}

void display_init(const DisplayPins_t *pins, spi_device_handle_t *spi_out)
{
    esp_err_t ret;
    spi_device_handle_t spi;

    // Конфиг SPI шины
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = pins->mosi,
        .miso_io_num = -1, // Не используем MISO
        .sclk_io_num = pins->clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * 320 * 2 + 8, // DMA-буфер под дисплей
    };

    // Инициализация SPI
    ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // Конфиг устройства (дисплей)
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 32 * 1000 * 1000, // 32 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = pins->cs,
        .queue_size = 7,                     // Размер очереди транзакций
        .pre_cb = spi_pre_transfer_callback, // Колбэк для DC пина
    };

    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi);
    ESP_ERROR_CHECK(ret);

    // Инициализация дисплея
    TFT9341_ini(spi, 320, 240);
    TFT9341_FillScreen(spi, TFT9341_BLACK);

    *spi_out = spi; // Сохраняем хэндл в переданный указатель
}

void app_main()
{
    DisplayPins_t pins = {18, 23, 27, 26, 25}; // Пины
    spi_device_handle_t spi;                   // Делаем хэндл SPI глобальным для файла

    // Передаем &spi в display_init, чтобы он его заполнил
    display_init(&pins, &spi); // Теперь функция принимает два аргумента

    TFT9341_SetRotation(spi, 3);
    TFT9341_SetTextColor(TFT9341_GREEN);
    TFT9341_SetBackColor(TFT9341_BLACK);
    TFT9341_SetFont(&Font12);

    while (1)
    {
        // Отрисовка строк с задержкой
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