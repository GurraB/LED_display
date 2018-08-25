#ifndef clock_h
#define clock_h
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <inttypes.h>

uint8_t update_clock();

void get_time();

void init_clock(EventGroupHandle_t wifi_event_group);

void obtain_time(EventGroupHandle_t wifi_event_group);

#endif