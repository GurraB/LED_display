#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "weather.h"
#include "freertos/event_groups.h"

#define WEATHER_TAG "WEATHER"

float temperature = 99;

esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(WEATHER_TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

float get_temperature() {
    return temperature;
}

void update_weather_information(EventGroupHandle_t wifi_event_group) {
    xEventGroupWaitBits(wifi_event_group, BIT0, false, true, portMAX_DELAY);
    char url[200];
    sprintf(url, API_CALL, MALMOE_ID, API_KEY);
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handle,
        .buffer_size = 1024,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(WEATHER_TAG, "Status = %d, content_length = %d",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));
        
        char response_buffer[1024];
        esp_http_client_read(client, response_buffer, sizeof(response_buffer));
        extract_temperature(response_buffer);
    }
    esp_http_client_cleanup(client);
}

void extract_temperature(char* get_response) {
    char str_temperature[5];
    char* temp_pointer = strstr(get_response, "\"temp\":");
    if(temp_pointer != NULL) {
        temp_pointer += 7;
        uint8_t counter = 0;
        while(counter < 5) {
            if(*temp_pointer == ',') {
                break;
            }
            str_temperature[counter] = *temp_pointer++;
            counter++;
        }
        temperature = strtof(str_temperature, NULL);

    } else {
        ESP_LOGE(WEATHER_TAG, "Could not find \"temp\": in response");
        printf("Response is:\n %s\n\n", get_response);
    }
}