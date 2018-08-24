
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


#define TAG "LED_DISPLAY"
#define OTA_TAG "OTA"
#define MAINPROCESSOR 0
#define ULPPROCESSOR 1

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

void clock_task(void* pvParameter) {
    uint32_t t = 2500 * 9;
    uint8_t s0, s0_old = 0, s1, s1_old = 0, m0, m0_old = 0, m1, m1_old = 0, h0, h0_old = 0, h1, h1_old = 0;
    uint8_t digit_r[15] = {0};
    uint8_t digit_g[15] = {0};
    uint8_t digit_b[15] = {0};
    
    get_digit(0, digit_b);
    while(1) {
        s0 = (t % 60) % 10;
        s1 = (t % 60) / 10;
        m0 = ((t / 60) % 60) % 10;
        m1 = ((t / 60) % 60) / 10;
        h0 = ((t / 3600) % 3600) % 10;
        h1 = ((t / 3600) % 3600) / 10;
        printf("%d%d:%d%d:%d%d\n", h1, h0, m1, m0, s1, s0);
        t++;
        for(uint8_t i = 0; i < 8; i++) {
            if(h1 == h1_old) {
                get_digit(h1, digit_b);
                draw_digit(0, 0, digit_r, digit_g, digit_b);
            } else {
                get_single_animation_step(h1_old, h1, i, digit_b);
                draw_digit(0, 0, digit_r, digit_g, digit_b);
            }
            if(h0 == h0_old) {
                get_digit(h0, digit_b);
                draw_digit(9, 0, digit_r, digit_g, digit_b);
            } else {
                get_single_animation_step(h0_old, h0, i, digit_b);
                draw_digit(9, 0, digit_r, digit_g, digit_b);
            }
            set_pixel(18, 6, 0, 0, 1);
            set_pixel(18 ,8, 0, 0, 1);
            if(m1 == m1_old) {
                get_digit(m1, digit_b);
                draw_digit(20, 0, digit_r, digit_g, digit_b);
            } else {
                get_single_animation_step(m1_old, m1, i, digit_b);
                draw_digit(20, 0, digit_r, digit_g, digit_b);
            }
            if(m0 == m0_old) {
                get_digit(m0, digit_b);
                draw_digit(29, 0, digit_r, digit_g, digit_b);
            } else {
                get_single_animation_step(m0_old, m0, i, digit_b);
                draw_digit(29, 0, digit_r, digit_g, digit_b);
            }
            set_pixel(38, 6, 0, 0, 1);
            set_pixel(38 ,8, 0, 0, 1);
            if(s1 == s1_old) {
                get_digit(s1, digit_b);
                draw_digit(40, 0, digit_r, digit_g, digit_b);
            } else {
                get_single_animation_step(s1_old, s1, i, digit_b);
                draw_digit(40, 0, digit_r, digit_g, digit_b);
            }
            get_single_animation_step(s0_old, s0, i, digit_b);
            draw_digit(49, 0, digit_r, digit_g, digit_b);
            
            update_display();
            vTaskDelay(30 / portTICK_RATE_MS);
        }

        s0_old = s0;
        s1_old = s1;
        m0_old = m0;
        m1_old = m1;
        h0_old = h0;
        h1_old = h1;
        vTaskDelay(760 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void draw_display_task(void* pvParameter) {
    while(1) {
        draw_display();
        vTaskDelay(1 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void test_task(void* pvParamter) {
    uint16_t pos = 0;
    uint8_t led = 0;
    uint16_t brightness = 1000;
    int8_t change = -1;
    while(1) {
        if(led == 0)
            set_pixel((pos)%64, pos/64, 1, 0, 0);
        else if(led == 1)
            set_pixel((pos)%64, pos/64, 0, 1, 0);
        else
            set_pixel((pos)%64, pos/64, 0, 0, 1);
        //set_brightness(brightness/10);
        pos++;
        brightness += change;
        if(pos > 64 * 32 - 1) {
            pos = 0;
            led++;
        }
        if(led >= 3)
            led = 0;
        if(brightness == 0)
            change = 1;
        if(brightness == 1000)
            change = -1;
        update_display();
        vTaskDelay(20 / portTICK_RATE_MS);
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
    wifi_event_group = initialise_wifi();
}

void app_main()
{
    init();
    xTaskCreatePinnedToCore(&web_server_task, "web_server_task", 16384, NULL, 1, NULL, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&clock_task, "clock_task", 8192, NULL, 1, NULL, MAINPROCESSOR);
    //xTaskCreatePinnedToCore(&test_task, "test_task", 8192, NULL, 1, NULL, MAINPROCESSOR);
    xTaskCreatePinnedToCore(&draw_display_task, "draw_display_task", 16384, NULL, 1, NULL, ULPPROCESSOR);
}

