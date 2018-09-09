
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_task_wdt.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "http_server.h"
#include <sys/param.h>

#include "driver/gpio.h"

#include "rgb_matrix.h"
#include "web_server.h"
#include "wifi_connection.h"
#include "morphing_digits.h"
#include "clock.h"
#include "weather.h"

#define TAG "LED_DISPLAY"
#define OTA_TAG "OTA"
#define MAINPROCESSOR 0
#define ULPPROCESSOR 1

#define MAX_DISPLAY_UPDATE_FREQ (10 / portTICK_RATE_MS)

TaskHandle_t clock_task_handle;

EventGroupHandle_t wifi_event_group;

static uint8_t local_buffer[BUFFER_LENGTH]; 

const int CONNECTED_BIT = BIT0;

char ota_update_address[100];
char ota_ca_cert[1536] = "";

void ota_update_task(void* pvParameter)
{
    //char* address = (char*) pvParameter;
    ESP_LOGI(OTA_TAG, "OTA download address: %s", ota_update_address);
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    printf((char *)ota_ca_cert);

    esp_http_client_config_t config = {
        .url = ota_update_address,
        .cert_pem = ota_ca_cert,
    };
    // download the new firmware
    esp_err_t status = esp_https_ota(&config);
    if (status == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(OTA_TAG, "Firmware Upgrades Failed");
    }
    // delete this task
    vTaskDelete(NULL);
}

void ota_callback(char* address) {
    ESP_LOGI(OTA_TAG, "OTA_CALLBACK, address: %s\n", address);
    strcpy(ota_update_address, address);
    xTaskCreate(&ota_update_task, "ota_update_task", 8192, NULL, 5, NULL);
}

void ota_renew_pem_callback(char* pem) {
    printf("OTA_UPDATE_PEM\n%s\n", pem);
    strcpy(ota_ca_cert, pem);
}

void web_server_task(void* pvParameter) {
    printf("Web server task started\n");
    // wait till the CONNECTED_BIT is set to 1
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    
    ota_callback_register(ota_callback);
    ota_renew_pem_callback_register(ota_renew_pem_callback);
    // start the web server
    esp_err_t status = start_web_server();
    ESP_ERROR_CHECK(status);

    // delete this task
    vTaskDelete(NULL);
}

void clock_task(void* pvParameter) {
    TickType_t last_wake_time = xTaskGetTickCount();
    struct rgb_color time_color = get_color(0, 0, 32);
    struct rgb_color date_color = get_color(32, 32, 0);
    struct rgb_color temperature_color = get_color(0, 32, 0);
    while(1) {
        update_clock(time_color, date_color, temperature_color, 0, 0, REGULAR);
        vTaskDelayUntil(&last_wake_time, 1000 / portTICK_RATE_MS);  //wait 1s
    }
    vTaskDelete(NULL);
}

void weather_task(void* pvParameter) {
    TickType_t last_wake_time = xTaskGetTickCount();
    while(1) {
        update_weather_information(wifi_event_group);
        vTaskDelayUntil(&last_wake_time, (1000 * 60 * 15) / portTICK_RATE_MS); // update every 15 min
    }
    vTaskDelete(NULL);
}

void draw_display_task(void* pvParameter) {
    printf("Draw display task started\n");
    esp_task_wdt_delete(NULL);
    while(1) {
        draw_display();
    }
    vTaskDelete(NULL);
}

void test_task(void* pvParamter) {
    printf("Test task started\n");
    uint16_t pos = 0;
    struct rgb_color color = get_color(108, 62, 209);
    while(1) {
        
        for(uint8_t i = 0; i < 64; i++) {
            set_pixel_in_buffer(i, pos, color, local_buffer);
        }
        pos++;
        if(pos > 31)
            pos = 0;
        set_display(local_buffer);
        update_display();
        vTaskDelay(MAX_DISPLAY_UPDATE_FREQ);
    }
}

void init() {
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    init_rgb_matrix();
    init_morphing_digits();
    set_brightness(5);
    wifi_event_group = initialise_wifi();
    init_clock(wifi_event_group);
    xTaskHandle idle_handle = xTaskGetIdleTaskHandleForCPU(1);
    esp_task_wdt_delete(idle_handle); // the second CPU is dedicated to draw the display, therefore we don't want wdt on IDLE task
}

void app_main()
{
    init();
    xTaskCreatePinnedToCore(&web_server_task, "web_server_task", 16384, NULL, 1, NULL, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&clock_task, "clock_task", 8192, NULL, 2, &clock_task_handle, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&weather_task, "weather_task", 8192, NULL, 2, &clock_task_handle, MAINPROCESSOR);
    //xTaskCreatePinnedToCore(&test_task, "test_task", 16384, NULL, 2, NULL, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&draw_display_task, "draw_display_task", 16384, NULL, 1, NULL, ULPPROCESSOR);
}

