/* Подключаемые библиотеки */
#include "main.h"
#include "esp_timer.h"
#include "driver/gpio.h"

/* Используемые макросы */
#define GPIO_4 4
#define GPIO_LED GPIO_4
#define GPIO_CONTROL 21 // Пин для управления периодом

/* Объявление глобальных переменных */
uint8_t led_on = 1;
esp_timer_handle_t periodic_timer; // Делаем глобальной для управления
uint64_t current_period = 500000;  // Текущий период в микросекундах

/* Объявление функций */
static void gpio_init(void);
static void gpio_switch(gpio_num_t gpio_num);
static void periodic_timer_callback(void *arg);
static void timer_init(uint64_t period);
static void check_period_change(void);
static void update_timer_period(uint64_t new_period);

/* Точка входа программы */
void app_main(void)
{
    printf("🚀 Запуск программы с динамическим изменением периода таймера\n");
    printf("📌 GPIO4 - светодиод, GPIO21 - управление периодом\n");

    gpio_init();
    timer_init(500000); // Начальный период 500ms

    printf("✅ Инициализация завершена. Начальный период: 500ms\n");
    printf("🔧 Логика: GPIO21=0 -> 500ms, GPIO21=1 -> 100ms\n\n");

    while (1)
    {
        check_period_change();         // Проверяем изменение состояния GPIO21
        vTaskDelay(pdMS_TO_TICKS(50)); // Проверяем каждые 50ms
    }
}

/* Функция инициализации GPIO */
static void gpio_init(void)
{
    printf("🛠️ Инициализация GPIO...\n");

    // Инициализация светодиода
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(GPIO_LED, 0);
    printf("   ✅ GPIO%d - светодиод инициализирован\n", GPIO_LED);

    // Инициализация пина управления
    gpio_reset_pin(GPIO_CONTROL);
    gpio_set_direction(GPIO_CONTROL, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_CONTROL, GPIO_PULLUP_ONLY); // Подтяжка к питанию
    printf("   ✅ GPIO%d - пин управления периодом (с подтяжкой)\n", GPIO_CONTROL);

    printf("   📊 Начальное состояние GPIO21: %d\n", gpio_get_level(GPIO_CONTROL));
}

/* Функция инициализации таймера */
static void timer_init(uint64_t period)
{
    printf("🛠️ Создание таймера...\n");

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "dynamic_led_timer"};

    esp_err_t ret = esp_timer_create(&periodic_timer_args, &periodic_timer);
    if (ret != ESP_OK)
    {
        printf("❌ Ошибка создания таймера: %s\n", esp_err_to_name(ret));
        return;
    }

    ret = esp_timer_start_periodic(periodic_timer, period);
    if (ret != ESP_OK)
    {
        printf("❌ Ошибка запуска таймера: %s\n", esp_err_to_name(ret));
        return;
    }

    current_period = period;
    printf("   ✅ Таймер создан и запущен\n");
    printf("   ⏱️  Начальный период: %lld мкс (%lld мс)\n", period, period / 1000);
}

/* Функция переключения состояния GPIO */
static void gpio_switch(gpio_num_t gpio_num)
{
    int current_level = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, !current_level);
}

/* Обработчик прерываний периодического таймера */
static void periodic_timer_callback(void *arg)
{
    static uint32_t counter = 0;
    counter++;

    if (led_on == 1)
    {
        int old_level = gpio_get_level(GPIO_LED); // Сохраняем текущее состояние
        gpio_switch(GPIO_LED);                    // Переключаем
        int new_level = gpio_get_level(GPIO_LED); // Получаем новое состояние

        printf("🔔 Таймер сработал #%lu | Период: %lldms | GPIO4: %d->%d | GPIO21: %d\n",
               counter, current_period / 1000, old_level, new_level, gpio_get_level(GPIO_CONTROL));
    }
}

/* Проверка изменения состояния пина управления */
static void check_period_change(void)
{
    static int last_control_state = -1; // Начальное неизвестное состояние
    int current_control_state = gpio_get_level(GPIO_CONTROL);

    // Если состояние изменилось
    if (current_control_state != last_control_state)
    {
        printf("\n🔄 Обнаружено изменение состояния GPIO21: %d -> %d\n",
               last_control_state, current_control_state);

        uint64_t new_period;
        const char *period_name;

        if (current_control_state == 0)
        {
            new_period = 500000; // 500ms
            period_name = "МЕДЛЕННЫЙ (500ms)";
        }
        else
        {
            new_period = 50000; // 50ms
            period_name = "БЫСТРЫЙ (50ms)";
        }

        printf("   🎯 Переключаем на %s режим\n", period_name);
        update_timer_period(new_period);

        last_control_state = current_control_state;
    }
}

/* Обновление периода таймера */
static void update_timer_period(uint64_t new_period)
{
    if (new_period == current_period)
    {
        printf("   ⏭️  Период не изменился, пропускаем\n");
        return;
    }

    printf("   🛠️  Обновление периода таймера: %lldms -> %lldms\n",
           current_period / 1000, new_period / 1000);

    // Останавливаем текущий таймер
    esp_err_t ret = esp_timer_stop(periodic_timer);
    if (ret != ESP_OK)
    {
        printf("   ❌ Ошибка остановки таймера: %s\n", esp_err_to_name(ret));
        return;
    }
    printf("   ✅ Таймер остановлен\n");

    // Перезапускаем с новым периодом
    ret = esp_timer_start_periodic(periodic_timer, new_period);
    if (ret != ESP_OK)
    {
        printf("   ❌ Ошибка перезапуска таймера: %s\n", esp_err_to_name(ret));
        return;
    }

    current_period = new_period;
    printf("   ✅ Таймер перезапущен с новым периодом: %lldms\n", new_period / 1000);

    // Выводим информацию о таймере
    printf("   📊 Текущая статистика таймера:\n");
    esp_timer_dump(stdout);
    printf("\n");
}