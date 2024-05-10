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

    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "Opening file");
    FILE *f = fopen("/spiffs/index.html", "rb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char str1[500];
    size_t n;
    n = fread(str1, 1, sizeof(str1), f);
    fclose(f);
    str1[n] = 0;

    ESP_LOGI(TAG, "Read from file:\r\n%s", str1);

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

/* Обработчик прерываний периодического таймера */
static void periodic_timer_callback(void *arg)
{
    if (led_on == 1)
    {
        gpio_switch(GPIO_LED);
    }
}