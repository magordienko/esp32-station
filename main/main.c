/* Подключаемые библиотеки */
#include "main.h"

/* Используемые макросы */
#define GPIO_4 4
#define GPIO_LED GPIO_4

/* Объявление глобальных переменных */
uint8_t led_on = 1;

/* Объявление функций */
static void gpio_init(void);
static void gpio_switch(gpio_num_t gpio_num);
static void periodic_timer_callback(void *arg);
static void timer_init(uint64_t period);

/* Точка входа программы */
void app_main(void)
{
    gpio_init();
    timer_init(500000);
    while (1)
    {
    }
}

/* Функция инициализации GPIO */
static void gpio_init(void)
{
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_INPUT_OUTPUT);
}

/* Функция инициализации таймера */
static void timer_init(uint64_t period)
{
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "periodic"};
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, period);
}

/* Функция переключения состояния GPIO */
static void gpio_switch(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, !gpio_get_level(gpio_num));
}

/* Обработчик прерываний периодического таймера */
static void periodic_timer_callback(void *arg)
{
    if (led_on == 1)
    {
        gpio_switch(GPIO_LED);
    }
}