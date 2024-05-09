#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_

#include <string.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "gpio.h"
#include "timer.h"
#include "wifi.h"

/* Объявление функций */
static void gpio_init(void);
static void timer_init(uint64_t period);
static void periodic_timer_callback(void *arg);

#endif