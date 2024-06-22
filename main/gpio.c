#include "gpio.h"
#include "main.h"

/* Функция инициализации GPIO */
void gpio_init(void)
{
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(GPIO_LED, 0);
}

/* Функция переключения состояния GPIO */
void gpio_switch(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, !gpio_get_level(gpio_num));
}