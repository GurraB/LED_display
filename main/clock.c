#include "clock.h"
#include <inttypes.h>
#include <time.h>
#include "esp_log.h"
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "apps/sntp/sntp.h"

#include "rgb_matrix.h"
#include "morphing_digits.h"
#include "weather.h"

#define CLOCK_TAG "CLOCK_TAG"
//#ifndef CONNECTED_BIT
//#define CONNECTED_BIT BIT0
//#endif

#define TIME_SET_BIT BIT0
static EventGroupHandle_t time_event_group;

uint8_t s0, s0_old = 0, s1, s1_old = 0, m0, m0_old = 0, m1, m1_old = 0, h0, h0_old = 0, h1, h1_old = 0, day0, day1, month0, month1;

time_t now;
struct tm timeinfo;

uint8_t update_clock(struct rgb_color time_color, struct rgb_color date_color, struct rgb_color temperature_color, uint8_t x, uint8_t y, uint8_t digit_size) {
    if(!(xEventGroupGetBits(time_event_group) & TIME_SET_BIT)) {
        // TODO: draw a loading icon instead
        return 0;
    }
    struct rgb_color color_digit[digit_size * 8];
    get_time();
    
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

    uint8_t xpos = x;
    uint8_t offset_per_digit = 0;
    uint8_t animation_length = 5;
    uint8_t dots_offset = 0;
    if(digit_size == SMALL) {
        offset_per_digit = 5;
        animation_length = 5;
        dots_offset = 2;
    } else if(digit_size == REGULAR) {
        offset_per_digit = 9;
        animation_length = 9;
        dots_offset = 6;
    }

    for(uint8_t i = 1; i < animation_length; i++) {
        xpos = x;
        if(h1 == h1_old) {
            get_digit(h1, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        } else {
            get_single_animation_step(h1_old, h1, i, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        }
        xpos += offset_per_digit;
        if(h0 == h0_old) {
            get_digit(h0, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        } else {
            get_single_animation_step(h0_old, h0, i, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        }
        xpos += offset_per_digit;
        set_pixel(xpos, y + dots_offset, time_color);
        set_pixel(xpos , y + dots_offset + 2, time_color);
        xpos += 2;
        if(m1 == m1_old) {
            get_digit(m1, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        } else {
            get_single_animation_step(m1_old, m1, i, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        }
        xpos += offset_per_digit;
        if(m0 == m0_old) {
            get_digit(m0, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        } else {
            get_single_animation_step(m0_old, m0, i, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        }
        xpos += offset_per_digit;
        set_pixel(xpos, y + dots_offset, time_color);
        set_pixel(xpos ,y + dots_offset + 2, time_color);
        xpos += 2;
        if(s1 == s1_old) {
            get_digit(s1, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        } else {
            get_single_animation_step(s1_old, s1, i, color_digit, time_color, digit_size);
            draw_digit(xpos, y, color_digit, digit_size);
        }
        xpos += offset_per_digit;
        get_single_animation_step(s0_old, s0, i, color_digit, time_color, digit_size);
        draw_digit(xpos, y, color_digit, digit_size);

        xpos = x;
        xpos += draw_date(date_color, x, y + digit_size + 2, SMALL);
        draw_temperature(temperature_color, xpos, y + digit_size + 2, REGULAR);

        update_display();
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    
    s0_old = s0;
    s1_old = s1;
    m0_old = m0;
    m1_old = m1;
    h0_old = h0;
    h1_old = h1;
    return 1;
}

uint8_t draw_date(struct rgb_color color, uint8_t x, uint8_t y, uint8_t digit_size) {
    if(!(xEventGroupGetBits(time_event_group) & TIME_SET_BIT)) {
        // TODO: draw a loading icon instead
        return 0;
    }
    struct rgb_color color_digit[digit_size * 8];
    get_time();

    day0 = timeinfo.tm_mday % 10;
    day1 = timeinfo.tm_mday / 10;
    month0 = timeinfo.tm_mon % 10;
    month1 = timeinfo.tm_mon / 10;

    uint8_t xpos = x;
    uint8_t offset_per_digit = 0;
    if(digit_size == SMALL) {
        offset_per_digit = 5;
    } else if(digit_size == REGULAR) {
        offset_per_digit = 9;
    }

    get_digit(day1, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;
    get_digit(day0, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;

    if(digit_size == SMALL) {
        set_pixel(xpos + 2, y, color);
        set_pixel(xpos + 2, y + 1, color);
        set_pixel(xpos + 1, y + 2, color);
        set_pixel(xpos + 1, y + 3, color);
        set_pixel(xpos + 1, y + 4, color);
        set_pixel(xpos, y + 5, color);
        set_pixel(xpos, y + 6, color);
        xpos += 4;
    } else if(digit_size == REGULAR) {
        set_pixel(xpos + 4, y, color);
        set_pixel(xpos + 4, y + 1, color);
        set_pixel(xpos + 4, y + 2, color);
        set_pixel(xpos + 3, y + 3, color);
        set_pixel(xpos + 3, y + 4, color);
        set_pixel(xpos + 3, y + 5, color);
        set_pixel(xpos + 2, y + 6, color);
        set_pixel(xpos + 2, y + 7, color);
        set_pixel(xpos + 2, y + 8, color);
        set_pixel(xpos + 1, y + 9, color);
        set_pixel(xpos + 1, y + 10, color);
        set_pixel(xpos + 1, y + 11, color);
        set_pixel(xpos, y + 12, color);
        set_pixel(xpos, y + 13, color);
        set_pixel(xpos, y + 14, color);
        xpos += 6;
    }

    get_digit(month1, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;
    get_digit(month0, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;

    return xpos - x;
}

uint8_t draw_temperature(struct rgb_color color, uint8_t x, uint8_t y, uint8_t digit_size) {
    struct rgb_color color_digit[digit_size * 8];
    uint8_t offset_per_digit = 0;
    uint8_t dot_offset = 0;
    uint8_t negative_offset = 0;
    uint8_t xpos = x;
    uint8_t degree_offset = 0;
    uint8_t digit;
    if(digit_size == SMALL) {
        offset_per_digit = 5;
        dot_offset = 6;
        negative_offset = 3;
        degree_offset = 0;
    } else if(digit_size == REGULAR) {
        offset_per_digit = 9;
        dot_offset = 14;
        negative_offset = 7;
        degree_offset = 6;
    }
    float temperature = get_temperature();
    if(temperature < 0) {
        set_pixel(xpos, y + negative_offset, color);
        set_pixel(xpos + 1, y + negative_offset, color);
        if(digit_size == REGULAR) {
            set_pixel(xpos + 2, y + negative_offset, color);
            xpos++;
        }
        xpos += 3;
    }
    if(temperature >= 10 || temperature <= -10) {
        digit = (uint8_t) abs(floor(temperature / 10));
        get_digit(digit, color_digit, color, digit_size);
        draw_digit(xpos, y, color_digit, digit_size);
        xpos += offset_per_digit;
    }
    digit = (uint8_t) abs(((uint8_t) floor(temperature) % 10));
    get_digit(digit, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;

    set_pixel(xpos, y + dot_offset, color);
    xpos += 2;

    digit = abs(((uint8_t) round(temperature * 10))) % 10;
    get_digit(digit, color_digit, color, digit_size);
    draw_digit(xpos, y, color_digit, digit_size);
    xpos += offset_per_digit;

    if(digit_size == SMALL) {

    } else if(digit_size == REGULAR) {
        set_pixel(xpos, y + degree_offset + 1, color);
        set_pixel(xpos, y + degree_offset + 2, color);
        set_pixel(xpos + 1, y + degree_offset, color);
        set_pixel(xpos + 2, y + degree_offset, color);
        set_pixel(xpos + 1, y + degree_offset + 3, color);
        set_pixel(xpos + 2, y + degree_offset + 3, color);
        set_pixel(xpos + 3, y + degree_offset + 1, color);
        set_pixel(xpos + 3, y + degree_offset + 2, color);
        xpos += 4;
        
        set_pixel(xpos + 1, y + degree_offset + 4, color);
        set_pixel(xpos + 2, y + degree_offset + 4, color);
        set_pixel(xpos + 3, y + degree_offset + 4, color);
        set_pixel(xpos, y + degree_offset + 5, color);
        set_pixel(xpos, y + degree_offset + 6, color);
        set_pixel(xpos, y + degree_offset + 7, color);
        set_pixel(xpos + 1, y + degree_offset + 8, color);
        set_pixel(xpos + 2, y + degree_offset + 8, color);
        set_pixel(xpos + 3, y + degree_offset + 8, color);
        xpos += 5;
    }

    return xpos;
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
    xEventGroupWaitBits(wifi_event_group, BIT0, false, true, portMAX_DELAY);
    ESP_LOGI(CLOCK_TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}