/* Подключаемые библиотеки */
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Используемые макросы */
#define GPIO_4 4
#define GPIO_LED GPIO_4

/* Объявление функций */
static void gpio_init(void);
static void gpio_switch(gpio_num_t gpio_num);

/* Точка входа программы */
void app_main(void)
{
    gpio_init();
    while (1)
    {
        gpio_switch(GPIO_LED);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/* Функция инициализации GPIO */
static void gpio_init(void)
{
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_INPUT_OUTPUT);
}

/* Функция переключения состояния GPIO */
static void gpio_switch(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, !gpio_get_level(gpio_num));
}