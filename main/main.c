#include <stdio.h>
#include <inttypes.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define GPIO_2 2
#define GPIO_4 4

/* */
static void behavior_gpio(void)
{
    static uint8_t s_led_state = 0;

    s_led_state = !s_led_state;

    gpio_set_level(GPIO_2, s_led_state);
    gpio_set_level(GPIO_4, !s_led_state);
}

/* */
static void configure_gpio(void)
{
    gpio_reset_pin(GPIO_2);
    gpio_reset_pin(GPIO_4);

    gpio_set_direction(GPIO_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_4, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    configure_gpio();
    while (1)
    {
        behavior_gpio();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}