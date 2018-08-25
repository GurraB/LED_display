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

uint8_t s0, s0_old = 0, s1, s1_old = 0, m0, m0_old = 0, m1, m1_old = 0, h0, h0_old = 0, h1, h1_old = 0, day0, day1, month0, month1;
uint8_t digit[15] = {0};
uint8_t digit_empty[15] = {0};

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
    h0 = timeinfo.tm_hour % 10;
    h1 = timeinfo.tm_hour / 10;
    
    day0 = timeinfo.tm_mday % 10;
    day1 = timeinfo.tm_mday / 10;
    month0 = timeinfo.tm_mon % 10;
    month1 = timeinfo.tm_mon / 10;

    for(uint8_t i = 0; i < 8; i++) {
        if(h1 == h1_old) {
            get_digit(h1, digit);
            draw_digit(0, 0, digit_empty, digit_empty, digit);
        } else {
            get_single_animation_step(h1_old, h1, i, digit);
            draw_digit(0, 0, digit_empty, digit_empty, digit);
        }
        if(h0 == h0_old) {
            get_digit(h0, digit);
            draw_digit(9, 0, digit_empty, digit_empty, digit);
        } else {
            get_single_animation_step(h0_old, h0, i, digit);
            draw_digit(9, 0, digit_empty, digit_empty, digit);
        }
        set_pixel(18, 6, 0, 0, 1);
        set_pixel(18 ,8, 0, 0, 1);
        if(m1 == m1_old) {
            get_digit(m1, digit);
            draw_digit(20, 0, digit_empty, digit_empty, digit);
        } else {
            get_single_animation_step(m1_old, m1, i, digit);
            draw_digit(20, 0, digit_empty, digit_empty, digit);
        }
        if(m0 == m0_old) {
            get_digit(m0, digit);
            draw_digit(29, 0, digit_empty, digit_empty, digit);
        } else {
            get_single_animation_step(m0_old, m0, i, digit);
            draw_digit(29, 0, digit_empty, digit_empty, digit);
        }
        set_pixel(38, 6, 0, 0, 1);
        set_pixel(38 ,8, 0, 0, 1);
        if(s1 == s1_old) {
            get_digit(s1, digit);
            draw_digit(40, 0, digit_empty, digit_empty, digit);
        } else {
            get_single_animation_step(s1_old, s1, i, digit);
            draw_digit(40, 0, digit_empty, digit_empty, digit);
        }
        get_single_animation_step(s0_old, s0, i, digit);
        draw_digit(49, 0, digit_empty, digit_empty, digit);
        
        get_digit(day1, digit);
        draw_digit(0, 16, digit_empty, digit, digit_empty);
        get_digit(day0, digit);
        draw_digit(9, 16, digit_empty, digit, digit_empty);
        
        set_pixel(18, 30, 0, 1, 0);
        set_pixel(18, 29, 0, 1, 0);
        set_pixel(18, 28, 0, 1, 0);
        set_pixel(19, 27, 0, 1, 0);
        set_pixel(19, 26, 0, 1, 0);
        set_pixel(19, 25, 0, 1, 0);
        set_pixel(20, 24, 0, 1, 0);
        set_pixel(20, 23, 0, 1, 0);
        set_pixel(20, 22, 0, 1, 0);
        set_pixel(21, 21, 0, 1, 0);
        set_pixel(21, 20, 0, 1, 0);
        set_pixel(21, 19, 0, 1, 0);
        set_pixel(22, 18, 0, 1, 0);
        set_pixel(22, 17, 0, 1, 0);
        set_pixel(22, 16, 0, 1, 0);

        get_digit(month1, digit);
        draw_digit(24, 16, digit_empty, digit, digit_empty);
        get_digit(month0, digit);
        draw_digit(33, 16, digit_empty, digit, digit_empty);

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
    setenv("TZ", "GMT-2", 1);
    tzset();
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