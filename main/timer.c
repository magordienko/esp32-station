#include "timer.h"
#include "main.h"

/* Функция инициализации таймера */
void timer_init(uint64_t period)
{
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "periodic"};
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, period);
}

/* Обработчик прерываний периодического таймера */
void periodic_timer_callback(void *arg)
{
    gpio_switch(GPIO_LED);
}