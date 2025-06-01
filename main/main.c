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
    out_state->current_line = 0;

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
    state->current_line = 0; // Сбрасываем позицию курсора при очистке экрана
}

void display_reset_cursor(display_state_t *state)
{
    state->current_line = 0;
}

void display_print_wrapped(display_state_t *state, const char *text)
{
    char line_buffer[MAX_CHARS_PER_LINE + 1] = {0};
    const char *start = text;
    const char *end = text;
    const char *last_space = NULL;
    int line_length = 0;
    bool new_paragraph = true; // Флаг нового абзаца

    while (*start && state->current_line < MAX_LINES)
    {
        // Пропускаем разделители абзацев и пробелы в начале
        while ((*start == PARAGRAPH_DELIMITER || *start == ' ') && !new_paragraph)
            start++;

        // Если это новый абзац и не первая строка - добавляем интервал
        if (new_paragraph && state->current_line > 0)
        {
            state->current_line += PARAGRAPH_SPACING;
            if (state->current_line >= MAX_LINES)
            {
                ESP_LOGE(DISPLAY_TAG, "Screen overflow! Maximum lines reached.");
                break;
            }
        }

        // Находим конец текущего слова или разделитель
        end = start;
        last_space = NULL;
        line_length = 0;
        bool found_delimiter = false;

        while (*end && line_length < MAX_CHARS_PER_LINE && !found_delimiter)
        {
            if (*end == PARAGRAPH_DELIMITER)
            {
                found_delimiter = true;
                break;
            }
            if (*end == ' ')
            {
                last_space = end;
            }
            end++;
            line_length++;
        }

        // Если нашли разделитель абзаца
        if (found_delimiter)
        {
            // Копируем текст до разделителя
            int copy_len = end - start;
            if (copy_len > 0)
            {
                strncpy(line_buffer, start, copy_len);
                line_buffer[copy_len] = '\0';

                // Выводим строку с отступом для нового абзаца
                uint16_t x_pos = new_paragraph ? (LEFT_MARGIN + INDENT_WIDTH) : LEFT_MARGIN;
                display_draw_text(state, line_buffer, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT);
                state->current_line++;
            }

            // Устанавливаем флаг нового абзаца для следующей итерации
            new_paragraph = true;
            start = end + 1; // Пропускаем разделитель

            continue;
        }

        // Если мы достигли конца строки
        if (*end == '\0')
        {
            // Копируем оставшийся текст
            int copy_len = end - start;
            strncpy(line_buffer, start, copy_len);
            line_buffer[copy_len] = '\0';

            // Выводим строку с отступом для нового абзаца
            uint16_t x_pos = new_paragraph ? (LEFT_MARGIN + INDENT_WIDTH) : LEFT_MARGIN;
            display_draw_text(state, line_buffer, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT);
            state->current_line++;

            break;
        }

        // Если строка слишком длинная и нужно сделать перенос
        if (line_length >= MAX_CHARS_PER_LINE)
        {
            // Если есть пробел для переноса
            if (last_space != NULL && last_space > start)
            {
                // Копируем текст до пробела
                int copy_len = last_space - start;
                strncpy(line_buffer, start, copy_len);
                line_buffer[copy_len] = '\0';

                // Выводим строку с отступом
                uint16_t x_pos = new_paragraph ? (LEFT_MARGIN + INDENT_WIDTH) : LEFT_MARGIN;
                display_draw_text(state, line_buffer, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT);
                state->current_line++;
                new_paragraph = false;

                // Перемещаем указатель
                start = last_space + 1;
            }
            else // Принудительный перенос
            {
                // Копируем максимально возможное количество символов
                strncpy(line_buffer, start, MAX_CHARS_PER_LINE);
                line_buffer[MAX_CHARS_PER_LINE] = '\0';

                // Выводим строку с отступом
                uint16_t x_pos = new_paragraph ? (LEFT_MARGIN + INDENT_WIDTH) : LEFT_MARGIN;
                display_draw_text(state, line_buffer, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT);
                state->current_line++;
                new_paragraph = false;

                // Перемещаем указатель
                start += MAX_CHARS_PER_LINE;
            }
        }

        // Проверяем, не вышли ли за пределы экрана
        if (state->current_line >= MAX_LINES)
        {
            ESP_LOGE(DISPLAY_TAG, "Screen overflow! Maximum lines reached.");
            break;
        }
    }
}

// Пример использования с разными типами текста
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

    while (1)
    {
        // 1. Проект (одна строка)
        display_print_wrapped(&display, "Project: ESP32-Station");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 2. Дата (одна строка)
        display_print_wrapped(&display, "Date: 01/06/2025 year");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 3. Текст с явными переносами (символ |)
        display_print_wrapped(&display, "First paragraph.|Second paragraph with indent.");
        vTaskDelay(pdMS_TO_TICKS(2000));

        display_print_wrapped(&display, " ");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 4. Длинный текст с автопереносами и абзацами
        const char *long_text =
            "This is a long text that demonstrates automatic word wrapping. "
            "The text should flow naturally across multiple lines.|"
            "New paragraphs are marked by PALKA symbol and will have indentation. "
            "The system handles both automatic wrapping and manual breaks.";

        display_print_wrapped(&display, long_text);
        vTaskDelay(pdMS_TO_TICKS(5000));
        display_clear(&display, TFT9341_BLACK);
    }
}