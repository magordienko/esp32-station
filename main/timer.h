#ifndef MAIN_TIMER_H_
#define MAIN_TIMER_H_

#include "esp_timer.h"

void timer_init(uint64_t period);
void periodic_timer_callback(void *arg);

#endif