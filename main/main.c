
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


#define TAG "LED_DISPLAY"
#define OTA_TAG "OTA"
#define MAINPROCESSOR 0
#define ULPPROCESSOR 1

//extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
//extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static EventGroupHandle_t wifi_event_group;

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

void update_display_task(void* pvParameter) {
    uint16_t pos = 0;
    uint8_t led = 0;
    while(1) {
        update_display();
        if(led == 0)
            set_pixel((pos)%64, pos/64, 1, 0, 0);
        else if(led == 1)
            set_pixel((pos)%64, pos/64, 0, 1, 0);
        else
            set_pixel((pos)%64, pos/64, 0, 0, 1);
        pos++;
        if(pos > 64 * 32 - 1) {
            pos = 0;
            led++;
        }
        if(led >= 3)
            led = 0;
        vTaskDelay(1 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
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
    wifi_event_group = initialise_wifi();
}

void app_main()
{
    init();
    xTaskCreatePinnedToCore(&web_server_task, "web_server_task", 16384, NULL, 1, NULL, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&update_display_task, "update_display_task", 16384, NULL, 1, NULL, ULPPROCESSOR);
}

