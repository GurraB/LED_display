#ifndef clock_h
#define clock_h
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <inttypes.h>
#include "rgb_matrix.h"

uint8_t update_clock(struct rgb_color color, uint8_t x, uint8_t y, uint8_t digit_size);

void get_time();

void init_clock(EventGroupHandle_t wifi_event_group);

void obtain_time(EventGroupHandle_t wifi_event_group);

#endif