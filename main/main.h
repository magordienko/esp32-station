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
#include "esp_spiffs.h"
#include "spiffs_config.h"
#include "sdkconfig.h"

#include "gpio.h"
#include "timer.h"
#include "wifi.h"
#include "http.h"

#define GPIO_LED 4

#define STA_SSID "DIR-615-935"
#define STA_PASSWORD "01230406"
#define WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define CONFIG_WIFI_SCAN_RSSI_THRESHOLD -127

void nvs_init(void);
void nvs_spiffs(void);
void app_netif_init(void);
void event_loop_create(void);

#endif