#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "spi_ili9341.h"

#define num_row(i) i * 12

#endif