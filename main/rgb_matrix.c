#include "rgb_matrix.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "soc/frc_timer_reg.h"
#include "soc/rtc.h"
#include <math.h>
#include "morphing_digits.h"

#define LED_TAG "LED_MATRIX"

#define UPDATED_BIT BIT0

#define READ_TIMER ((uint64_t) REG_READ(FRC_TIMER_COUNT_REG(1)))

#define GET_OFFSET(X, Y, COLOR) ((Y < 16) ? ((Y * 64 * 6) + COLOR + (X * 6) ) : (((Y - 16) * 64 * 6) + COLOR + 3 + (X * 6)) )

SemaphoreHandle_t buffer_semaphore;
EventGroupHandle_t update_display_handle;
#define NEW_DATA_BIT BIT0

static uint8_t buffer[BUFFER_LENGTH * 2];
uint8_t* buffer_ptr = buffer;
uint8_t* shared_buffer_ptr = buffer + BUFFER_LENGTH;

uint8_t brightness = 100;
uint8_t shared_brightness = 100;
uint8_t r0_val, b0_val, g0_val, r1_val, g1_val, b1_val;
uint64_t t0 = 0, t1, t2, t3;
uint32_t timer_ticks_per_us;
uint8_t mux;

void init_pins() {
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that should be outputs
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // set all pins low except for OE that should be high
    gpio_output_set((1<<OE), GPIO_OUTPUT_PIN_SEL & ~(1<<OE), GPIO_OUTPUT_PIN_SEL, 0x00);
}

void init_rgb_matrix() {
    init_pins();
    buffer_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(buffer_semaphore);
    update_display_handle = xEventGroupCreate();
    xEventGroupClearBits(update_display_handle, NEW_DATA_BIT);
    uint32_t apb_freq = rtc_clk_apb_freq_get();
    timer_ticks_per_us = apb_freq / 1000000;
}

void update_display() {
    xEventGroupSetBits(update_display_handle, NEW_DATA_BIT);
}

void draw_display() {
    t1 = READ_TIMER;
    for(uint8_t color_intensity_counter = 1; color_intensity_counter < 9; color_intensity_counter++) {
        volatile uint8_t* temp_ptr = buffer;
        t2 = READ_TIMER;
        for(uint8_t i = 0; i < 16; i++) {
            t3 = READ_TIMER;
            if(i == 0) {
                mux = 15;
            } else {
                mux = i - 1;
            }
            if(mux & 0x01) {
                GPIO.out_w1ts = (1 << A_pin);
            } else {
                GPIO.out_w1tc = (1 << A_pin);
            }
            if(mux & 0x02) {
                GPIO.out_w1ts = (1 << B_pin);
            } else {
                GPIO.out_w1tc = (1 << B_pin);
            }
            if(mux & 0x04) {
                GPIO.out_w1ts = (1 << C_pin);
            } else {
                GPIO.out_w1tc = (1 << C_pin);
            }
            if(mux & 0x08) {
                GPIO.out_w1ts = (1 << D_pin);
            } else {
                GPIO.out_w1tc = (1 << D_pin);
            }
            
            GPIO.out_w1ts = (1 << LAT);
            GPIO.out_w1tc = (1 << LAT);

            t0 = READ_TIMER;
            GPIO.out_w1tc = (1 << OE);

            for(uint8_t k = 0; k < 64; k++) {
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << r0_pin);
                } else {
                    GPIO.out_w1tc = (1 << r0_pin);
                }
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << g0_pin);
                } else {
                    GPIO.out_w1tc = (1 << g0_pin);
                }
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << b0_pin);
                } else {
                    GPIO.out_w1tc = (1 << b0_pin);
                }
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << r1_pin);
                } else {
                    GPIO.out_w1tc = (1 << r1_pin);
                }
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << g1_pin);
                } else {
                    GPIO.out_w1tc = (1 << g1_pin);
                }
                if((*temp_ptr++) >= color_intensity_counter) {
                    GPIO.out_w1ts = (1 << b1_pin);
                } else {
                    GPIO.out_w1tc = (1 << b1_pin);
                }
                
                GPIO.out_w1ts = (1 << CLK);
                GPIO.out_w1tc = (1 << CLK);
            }
            
            while(((READ_TIMER - t0) / timer_ticks_per_us) < ((LED_ON_TIME * brightness * 2)));
            //while(((READ_TIMER - t0) / timer_ticks_per_us) < (70));
            GPIO.out_w1ts = (1 << OE);
            //printf("%llu\n", ((READ_TIMER - t3) / timer_ticks_per_us));
            //while(((READ_TIMER - t3) / timer_ticks_per_us) < (50)); // making sure every line taskes the same amount of time to draw reduces flickering
        }
        
    }
    if(xEventGroupGetBits(update_display_handle) & NEW_DATA_BIT) {
        if(xSemaphoreTake(buffer_semaphore, (TickType_t) 20) == pdTRUE) {
            uint8_t* temp = buffer_ptr;
            buffer_ptr = shared_buffer_ptr;
            shared_buffer_ptr = temp;
            brightness = shared_brightness;
            xSemaphoreGive(buffer_semaphore);
            //clear_display();
            xEventGroupClearBits(update_display_handle, NEW_DATA_BIT);
        } else {
            ESP_LOGE(LED_TAG, "Failed to swap the buffers");
        }
    }
    printf("%llu\n", ((READ_TIMER - t1) / timer_ticks_per_us));
    while(((READ_TIMER - t1) / timer_ticks_per_us) < (5000)); // update the display every 1 ms
} // 5500

void set_pixel(uint8_t x, uint8_t y, struct rgb_color color) {
    // error check if the pixel is outside of panel, since it's unsigned integer, most unsigned values will be outside (iffy)
    if(x >= WIDTH || y >= HEIGHT)
        return;
    //printf("setting x:%d, y:%d\t%d %d %d\n", x, y, r, g, b);
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            shared_buffer_ptr[GET_OFFSET(x, y, RED)] = color.r;
            shared_buffer_ptr[GET_OFFSET(x, y, GREEN)] = color.g;
            shared_buffer_ptr[GET_OFFSET(x, y, BLUE)] = color.b;
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

void set_pixel_in_buffer(uint8_t x, uint8_t y, struct rgb_color color, uint8_t* buff) {
    // error check if the pixel is outside of panel, since it's unsigned integer, most unsigned values will be outside (iffy)
    if(x >= WIDTH || y >= HEIGHT)
        return;
    buff[GET_OFFSET(x, y, RED)] = color.r;
    buff[GET_OFFSET(x, y, GREEN)] = color.g;
    buff[GET_OFFSET(x, y, BLUE)] = color.b;
}

void set_display(uint8_t* buff) {
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            memcpy(shared_buffer_ptr, buff, BUFFER_LENGTH * sizeof(uint8_t));
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

void draw_digit(uint8_t x, uint8_t y, struct rgb_color* digit, uint8_t digit_size) {
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            for(uint8_t h = 0; h < digit_size; h++) {
                if(digit_size == SMALL) {
                    for(uint8_t w = 0; w < 4; w++) {
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), RED)] = (digit[(h * 8) + w].r);
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), GREEN)] = (digit[(h * 8) + w].g);
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), BLUE)] = (digit[(h * 8) + w].b);
                    }
                } else if(digit_size == REGULAR) {
                    for(uint8_t w = 0; w < 8; w++) {
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), RED)] = digit[(h * 8) + w].r;
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), GREEN)] = digit[(h * 8) + w].g;
                        shared_buffer_ptr[GET_OFFSET((x + w), (y + h), BLUE)] = digit[(h * 8) + w].b;
                    }
                }
            }
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

void clear_display() {
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            memset(shared_buffer_ptr, 0, BUFFER_LENGTH * sizeof(uint8_t));
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

void set_brightness(uint8_t brightness_val) {
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            shared_brightness = brightness_val;
            if(shared_brightness > 100)
                shared_brightness = 100;
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

struct rgb_color get_color(uint8_t r, uint8_t g, uint8_t b) {
    struct rgb_color color;
    color.r = (uint8_t) round(((float)r / 32));
    color.g = (uint8_t) round(((float)g / 32));
    color.b = (uint8_t) round(((float)b / 32));
    return color;
}