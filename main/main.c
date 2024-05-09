/* Подключаемые библиотеки */
#include "main.h"

/* Используемые макросы */
#define GPIO_4 4
#define GPIO_LED GPIO_4

/* Объявление глобальных переменных */
uint8_t led_on = 1;
static const char *TAG = "main";

void app_main(void)
{
    gpio_init();
    timer_init(500000);

    wifi_ap_record_t info;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_erase();
        ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
        ret = nvs_flash_init();
        ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
    }
    ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
    wifi_init_sta();

    while (1)
    {
        ESP_LOGI(TAG, "Heap free size:%d", xPortGetFreeHeapSize());
        ret = esp_wifi_sta_get_ap_info(&info);
        ESP_LOGI(TAG, "esp_wifi_sta_get_ap_info: 0x%04x", ret);
        if (ret == 0)
            ESP_LOGI(TAG, "SSID: %s", info.ssid);
        else
        {
            wifi_init_sta();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

/* Обработчик прерываний периодического таймера */
static void periodic_timer_callback(void *arg)
{
    if (led_on == 1)
    {
        gpio_switch(GPIO_LED);
    }
}