#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "main.h"
#include "http.h"

void wifi_sta_init(void);

#endif