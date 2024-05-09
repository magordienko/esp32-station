#include "gpio.h"

/* Функция переключения состояния GPIO */
void gpio_switch(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, !gpio_get_level(gpio_num));
}