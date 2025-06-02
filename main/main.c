#include "main.h"

static int display_dc_pin = -1;        // Храним DC-пин здесь
static uint16_t prev_sine_points[320]; // Массив для хранения предыдущих точек синусоиды

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
    out_state->font = &Font12rus;
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

void draw_sine_wave(display_state_t *state)
{
    const uint16_t width = 320;
    const uint16_t height = 240;
    const uint16_t center_y = height / 2;
    const uint16_t amplitude = 100; // Амплитуда синусоиды
    const int periods = 6;          // Количество периодов
    const uint16_t color = TFT9341_WHITE;

    // Очищаем экран
    display_clear(state, TFT9341_BLACK);

    // Рисуем синусоиду
    uint16_t prev_x = 0;
    uint16_t prev_y = center_y;

    for (uint16_t x = 0; x < width; x++)
    {
        // Вычисляем y-координату для текущего x
        double radians = (double)x / width * 2 * M_PI * periods;
        double sine_value = sin(radians);
        uint16_t y = center_y - (uint16_t)(sine_value * amplitude);

        // Рисуем линию от предыдущей точки к текущей
        TFT9341_DrawLine(state->spi, color, prev_x, prev_y, x, y);

        prev_x = x;
        prev_y = y;
    }
}

void draw_animated_sine_wave(display_state_t *state)
{
    const uint16_t width = 320;
    const uint16_t height = 240;
    const uint16_t center_y = height / 2;
    const uint16_t max_amplitude = 10; // Максимальная амплитуда
    const int carrier_periods = 20;    // 10 периодов несущей частоты
    const int envelope_periods = 2;    // 3 периода огибающей
    const uint16_t wave_color = TFT9341_WHITE;
    const uint16_t bg_color = TFT9341_BLACK;
    static float phase = 0.0f;

    // Рисуем новую синусоиду, стирая предыдущую
    for (uint16_t x = 0; x < width; x++)
    {
        // Стираем предыдущую точку
        TFT9341_DrawPixel(state->spi, x, prev_sine_points[x], bg_color);

        // Вычисляем позицию в радианах
        double x_rad = (double)x / width * 2 * M_PI;

        // Несущая высокая частота (10 периодов)
        double carrier = sin(x_rad * carrier_periods + phase);

        // Огибающая низкая частота (3 периода)
        double envelope = sin(x_rad * envelope_periods);

        // AM-сигнал: несущая × (1 + огибающая)/2
        double am_signal = carrier * (1.0 + envelope) / 2.0;

        // Масштабируем до нужной амплитуды
        uint16_t y = center_y - (uint16_t)(am_signal * max_amplitude);

        // Сохраняем точку для следующего кадра
        prev_sine_points[x] = y;

        // Рисуем новую точку
        TFT9341_DrawPixel(state->spi, x, y, wave_color);
    }

    // Обновляем фазу для анимации движения
    phase += 0.5f;
    if (phase > 2 * M_PI)
    {
        phase -= 2 * M_PI;
    }
}

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

    // Инициализация массива точек
    for (int i = 0; i < 320; i++)
    {
        prev_sine_points[i] = config.height / 2;
    }

    // Очищаем экран перед началом анимации
    display_clear(&display, TFT9341_BLACK);
    display_print_wrapped(&display, "Only long text in English can cover complex ideas thoroughly, but this one is brief by design-just twenty words DDDDDDDDDWWWWW. Eta function ne mojet v russkiy.");

    TFT9341_DrawUTF8String(display.spi, 13, 150, "Привет, World! Ахуеть как долго я фиксил"); // Смешанный русский/английский текст
    TFT9341_DrawUTF8String(display.spi, 13, 170, "Ростик - Пидор!");
    TFT9341_DrawUTF8String(display.spi, 13, 190, "ДАБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ");
    TFT9341_DrawUTF8String(display.spi, 13, 210, "абвгдежзийклмнопрстуфхцчшщъыьэюя");

    while (1)
    {
        draw_animated_sine_wave(&display);
        vTaskDelay(pdMS_TO_TICKS(10)); // Увеличили FPS до ~20
    }
}