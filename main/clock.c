#include "clock.h"
#include <inttypes.h>
#include <time.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "apps/sntp/sntp.h"

#include "rgb_matrix.h"
#include "morphing_digits.h"

#define CLOCK_TAG "CLOCK_TAG"
#ifndef CONNECTED_BIT
#define CONNECTED_BIT BIT0
#endif

#define TIME_SET_BIT BIT0
static EventGroupHandle_t time_event_group;

uint8_t s0, s0_old = 0, s1, s1_old = 0, m0, m0_old = 0, m1, m1_old = 0, h0, h0_old = 0, h1, h1_old = 0;
uint8_t digit_r[15] = {0};
uint8_t digit_g[15] = {0};
uint8_t digit_b[15] = {0};

time_t now;
struct tm timeinfo;

uint8_t update_clock() {
    get_time();
    
    if(!(xEventGroupGetBits(time_event_group) & TIME_SET_BIT)) {
        // TODO: draw a loading icon instead
        return 0;
    }
    s0 = timeinfo.tm_sec % 10;
    s1 = timeinfo.tm_sec / 10;
    m0 = timeinfo.tm_min % 10;
    m1 = timeinfo.tm_min / 10;
    h0 = (timeinfo.tm_hour + 2) % 10;
    h1 = (timeinfo.tm_hour + 2) / 10;

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
    return 1;
}

void get_time() {
    time(&now);
    localtime_r(&now, &timeinfo);
}

void init_clock(EventGroupHandle_t wifi_event_group) {
    time_event_group = xEventGroupCreate();
    xEventGroupClearBits(time_event_group, TIME_SET_BIT);
    obtain_time(wifi_event_group);
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(CLOCK_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if(retry < retry_count) {
        xEventGroupSetBits(time_event_group, TIME_SET_BIT);
    }
}

void obtain_time(EventGroupHandle_t wifi_event_group) {
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(CLOCK_TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "se.pool.ntp.org");
    sntp_init();
}