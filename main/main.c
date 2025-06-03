#include "main.h"

static void spi_pre_transfer_callback(spi_transaction_t *trans)
{
    int dc_level = (int)trans->user;
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

void draw_animated_sine_wave(display_state_t *state, sine_animation_state_t *anim_state)
{
    // Константы
    static const uint16_t width = 320;
    static const uint16_t height = 240;
    static const uint16_t center_y = height / 2;
    static const uint16_t max_amplitude = 10;
    static const int carrier_periods = 20;
    static const int envelope_periods = 2;
    static const uint16_t wave_color = TFT9341_WHITE;
    static const uint16_t bg_color = TFT9341_BLACK;
    static const float two_pi = 2.0f * M_PI;
    static const float half_pi = M_PI_2;
    static const float phase_step = 0.5f;
    static const float width_reciprocal = 1.0f / width;

    // Предварительно вычисленные константы
    const float carrier_scale = two_pi * carrier_periods * width_reciprocal;
    const float envelope_scale = two_pi * envelope_periods * width_reciprocal;

    for (uint16_t x = 0; x < width; x++)
    {
        // Стираем предыдущую точку
        TFT9341_DrawPixel(state->spi, x, anim_state->prev_sine_points[x], bg_color);

        // Вычисляем новую точку
        float x_pos = (float)x;
        float x_rad_carrier = x_pos * carrier_scale;
        float x_rad_envelope = x_pos * envelope_scale;

        float carrier = sinf(x_rad_carrier + anim_state->phase);
        float envelope = sinf(x_rad_envelope + half_pi);
        float am_signal = carrier * (0.5f + 0.5f * envelope);

        uint16_t y = center_y - (uint16_t)(am_signal * (float)max_amplitude);

        // Сохраняем точку для следующего кадра
        anim_state->prev_sine_points[x] = y;

        // Рисуем новую точку
        TFT9341_DrawPixel(state->spi, x, y, wave_color);
    }

    // Обновление фазы
    anim_state->phase += phase_step;
    if (anim_state->phase > two_pi)
    {
        anim_state->phase -= two_pi;
    }
}

void display_print_wrapped_rus(display_state_t *state, const char *text)
{
    char line_buffer[MAX_CHARS_PER_LINE * 2 + 1] = {0}; // Буфер с запасом для UTF-8
    const char *start = text;
    const char *end = text;
    const char *last_space = NULL;
    int line_length = 0;
    bool new_paragraph = true;

    // Устанавливаем шрифт и цвета
    TFT9341_SetTextColor(state->text_color);
    TFT9341_SetBackColor(state->bg_color);
    TFT9341_SetFont(state->font);

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

            // Определяем длину UTF-8 символа
            int char_len = ((*end & 0x80) == 0) ? 1 : 2;

            if (*end == ' ')
            {
                last_space = end;
            }

            end += char_len;
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
                TFT9341_DrawUTF8String(state->spi, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT, line_buffer);
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
            TFT9341_DrawUTF8String(state->spi, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT, line_buffer);
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
                TFT9341_DrawUTF8String(state->spi, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT, line_buffer);
                state->current_line++;
                new_paragraph = false;

                // Перемещаем указатель
                start = last_space + 1;
            }
            else // Принудительный перенос
            {
                // Копируем максимально возможное количество символов
                // С учетом UTF-8 символов
                const char *tmp = start;
                int char_count = 0;
                while (char_count < MAX_CHARS_PER_LINE && *tmp)
                {
                    int char_len = ((*tmp & 0x80) == 0) ? 1 : 2;
                    if (char_count + 1 > MAX_CHARS_PER_LINE)
                        break;
                    tmp += char_len;
                    char_count++;
                }

                strncpy(line_buffer, start, tmp - start);
                line_buffer[tmp - start] = '\0';

                // Выводим строку с отступом
                uint16_t x_pos = new_paragraph ? (LEFT_MARGIN + INDENT_WIDTH) : LEFT_MARGIN;
                TFT9341_DrawUTF8String(state->spi, x_pos, TOP_MARGIN + state->current_line * LINE_HEIGHT, line_buffer);
                state->current_line++;
                new_paragraph = false;

                // Перемещаем указатель
                start = tmp;
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

// Функция для получения даты сборки
char *get_build_date(void)
{
    static char date_buffer[30]; // Увеличили буфер для дня недели
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const char *weekdays[] = {"сб", "вс", "пн", "вт", "ср", "чт", "пт"};

    // Получаем дату компиляции
    char *build_date = __DATE__; // Формат: "MMM DD YYYY"

    // Парсим месяц
    int month = 0;
    for (int i = 0; i < 12; i++)
    {
        if (strncmp(build_date, months[i], 3) == 0)
        {
            month = i + 1;
            break;
        }
    }

    // Парсим день и год
    int day, year;
    sscanf(build_date + 3, "%d %d", &day, &year);

    // Вычисляем день недели (алгоритм Зеллера)
    if (month < 3)
    {
        month += 12;
        year--;
    }
    int weekday = (day + (13 * (month + 1)) / 5 + year + year / 4 - year / 100 + year / 400) % 7;

    // Форматируем дату в "DD.MM.YYYY (пн)"
    snprintf(date_buffer, sizeof(date_buffer), "%02d.%02d.%04d (%s)", day, month, year, weekdays[weekday]);

    return date_buffer;
}

// Функция для объединения строк
char *combine_strings(const char *str1, const char *str2)
{
    static char combined[100]; // Статический буфер для результата
    snprintf(combined, sizeof(combined), "%s%s", str1, str2);
    return combined;
}

void app_main()
{
    display_config_t config = {
        .spi_host = HSPI_HOST,
        .pins = {.clk = 18, .mosi = 23, .dc = 27, .rst = 26, .cs = 25},
        .width = 320,
        .height = 240};

    display_state_t display;
    sine_animation_state_t anim_state = {0};

    // Инициализация массива точек
    for (int i = 0; i < 320; i++)
    {
        anim_state.prev_sine_points[i] = config.height / 2;
    }

    if (display_init(&config, &display) != ESP_OK)
    {
        ESP_LOGE(DISPLAY_TAG, "Display initialization failed!");
        return;
    }

    TFT9341_SetRotation(display.spi, 3);
    display_clear(&display, TFT9341_BLACK);
    display_print_wrapped_rus(&display, "Проект: \"ESP32-Station\"");

    char *firmware_date = get_build_date();
    char *firmware_text = combine_strings("Дата сборки прошивки: ", firmware_date);
    display_print_wrapped_rus(&display, firmware_text);

    while (1)
    {
        draw_animated_sine_wave(&display, &anim_state);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}