#ifndef wifi_connection_h
#define wifi_connection_h
#include "esp_system.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"

esp_err_t event_handler(void *ctx, system_event_t *event);

EventGroupHandle_t initialise_wifi(void);

#endif