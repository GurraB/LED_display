#ifndef weather_h
#define weather_h

#include <inttypes.h>
#include "freertos/event_groups.h"

#define API_CALL "http://api.openweathermap.org/data/2.5/weather?id=%s&units=metric&APPID=%s"
#define MALMOE_ID "2692969"

double get_temperature();

void update_weather_information(EventGroupHandle_t wifi_event_group);

void extract_temperature(char* get_response);

#endif