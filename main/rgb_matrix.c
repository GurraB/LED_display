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

#define LED_TAG "LED_MATRIX"

#define UPDATED_BIT BIT0

static uint32_t buffer[BUFFER_LENGTH * 2];
uint32_t* buffer_ptr = buffer + BUFFER_LENGTH;
uint32_t* shared_buffer_ptr = buffer;
SemaphoreHandle_t buffer_semaphore;
SemaphoreHandle_t buffer_swap_semaphore;
uint8_t state = 0;
uint8_t brightness = 100;
uint8_t shared_brightness = 100;
uint8_t pos, shift, r0_val, b0_val, g0_val, r1_val, g1_val, b1_val;
uint64_t t0 = 0;
uint32_t timer_ticks_per_us;

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
    buffer_swap_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(buffer_semaphore);
    xSemaphoreGive(buffer_swap_semaphore);
    uint32_t apb_freq = rtc_clk_apb_freq_get();
    timer_ticks_per_us = apb_freq / 1000000;
}

void update_display() {
    if(xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE) {
        if(xSemaphoreTake(buffer_swap_semaphore, (TickType_t) 10) == pdTRUE) {
            uint32_t* temp = buffer_ptr;
            buffer_ptr = shared_buffer_ptr;
            shared_buffer_ptr = temp;
            brightness = shared_brightness;
            xSemaphoreGive(buffer_swap_semaphore);
        } else {
            ESP_LOGE(LED_TAG, "Failed to swap the buffers");
        }
        xSemaphoreGive(buffer_semaphore);
        clear_display();
    }
}

void draw_display() {
    if(xSemaphoreTake(buffer_swap_semaphore, (TickType_t) 5) == pdTRUE) {
        for(uint8_t i = 0; i < 16; i++) {
            if(i & 0x01) {
                GPIO.out_w1ts = (1 << A_pin);
            } else {
                GPIO.out_w1tc = (1 << A_pin);
            }
            if(i & 0x02) {
                GPIO.out_w1ts = (1 << B_pin);
            } else {
                GPIO.out_w1tc = (1 << B_pin);
            }
            if(i & 0x04) {
                GPIO.out_w1ts = (1 << C_pin);
            } else {
                GPIO.out_w1tc = (1 << C_pin);
            }
            if(i & 0x08) {
                GPIO.out_w1ts = (1 << D_pin);
            } else {
                GPIO.out_w1tc = (1 << D_pin);
            }
            t0 = (uint64_t) REG_READ(FRC_TIMER_COUNT_REG(1)); // get time when turning on leds
            
            GPIO.out_w1tc = (1 << OE);
            
            for(uint8_t k = 0; k < 64; k++) {
                    pos = i*2 + (k >> 5);
                    shift = k & 0b11111;
                    
                    if((buffer_ptr[pos + R0_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << r0_pin);
                    } else {
                        GPIO.out_w1tc = (1 << r0_pin);
                    }
                    if((buffer_ptr[pos + G0_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << g0_pin);
                    } else {
                        GPIO.out_w1tc = (1 << g0_pin);
                    }
                    if((buffer_ptr[pos + B0_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << b0_pin);
                    } else {
                        GPIO.out_w1tc = (1 << b0_pin);
                    }
                    if((buffer_ptr[pos + R1_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << r1_pin);
                    } else {
                        GPIO.out_w1tc = (1 << r1_pin);
                    }
                    if((buffer_ptr[pos + G1_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << g1_pin);
                    } else {
                        GPIO.out_w1tc = (1 << g1_pin);
                    }
                    if((buffer_ptr[pos + B1_OFFSET] >> shift) & 0x01) {
                        GPIO.out_w1ts = (1 << b1_pin);
                    } else {
                        GPIO.out_w1tc = (1 << b1_pin);
                    }
                    
                    GPIO.out_w1ts = (1 << CLK);
                    GPIO.out_w1tc = (1 << CLK);
            }
            // wait till it's time to turn off leds again
            while(((((uint64_t) REG_READ(FRC_TIMER_COUNT_REG(1))) - t0) / timer_ticks_per_us) < LED_ON_TIME);
            GPIO.out_w1ts = (1 << OE);

            GPIO.out_w1ts = (1 << LAT);
            GPIO.out_w1tc = (1 << LAT);
        }
        xSemaphoreGive(buffer_swap_semaphore);
    }
}

void set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    // error check if the pixel is outside of panel, since it's unsigned integer, most unsigned values will be outside (iffy)
    if(x >= WIDTH || y >= HEIGHT)
        return;
    //ESP_LOGI(LED_TAG, "setting x:%d, y:%d\t%d %d %d", x, y, r, g, b);

    uint8_t pos = (y * 2) + (x / 32);
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            shared_buffer_ptr[pos + R0_OFFSET] |= (r << (x%32));
            shared_buffer_ptr[pos + R0_OFFSET] &= ~((1-r) << (x%32));
            shared_buffer_ptr[pos + G0_OFFSET] |= (g << (x%32));
            shared_buffer_ptr[pos + G0_OFFSET] &= ~((1-g) << (x%32));
            shared_buffer_ptr[pos + B0_OFFSET] |= (b << (x%32));
            shared_buffer_ptr[pos + B0_OFFSET] &= ~((1-b) << (x%32));
            xSemaphoreGive(buffer_semaphore);
        }
    }
}

void set_multiple_pixels(uint8_t x, uint8_t y, uint8_t* r, uint8_t* g, uint8_t* b, size_t x_size, size_t y_size) {
    for(uint8_t i = 0; i < y_size; i++) {
        for(uint8_t j = 0; j < x_size; j++) {
            set_pixel(x + j, y + i, r[i*x_size + j], g[i*x_size + j], b[i*x_size + j]);
        }
    }
}

void draw_digit(uint8_t x, uint8_t y, uint8_t* r, uint8_t* g, uint8_t* b) {
    uint64_t digit_r[15];
    uint64_t digit_g[15];
    uint64_t digit_b[15];
    for(uint8_t h = 0; h < 15; h++) {
        digit_r[h] = ((unsigned long long)r[h] << (x));
        digit_g[h] = ((unsigned long long)g[h] << (x));
        digit_b[h] = ((unsigned long long)b[h] << (x));
    }
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            for(uint8_t h = 0; h < 15; h++) {
                shared_buffer_ptr[((y + h) * 2) + R0_OFFSET] |= (digit_r[h] & 0x00000000FFFFFFFF);
                shared_buffer_ptr[((y + h) * 2) + 1 + R0_OFFSET] |= (digit_r[h] >> 32);
                shared_buffer_ptr[((y + h) * 2) + G0_OFFSET] |= (digit_g[h] & 0x00000000FFFFFFFF);
                shared_buffer_ptr[((y + h) * 2) + 1 + G0_OFFSET] |= (digit_g[h] >> 32);
                shared_buffer_ptr[((y + h) * 2) + B0_OFFSET] |= (digit_b[h] & 0x00000000FFFFFFFF);
                shared_buffer_ptr[((y + h) * 2) + 1 + B0_OFFSET] |= (digit_b[h] >> 32);
            }
            xSemaphoreGive(buffer_semaphore);
        }
    }    
}

void clear_display() {
    if(buffer_semaphore != NULL) {
        if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
            memset(shared_buffer_ptr, 0, BUFFER_LENGTH * sizeof(uint32_t));
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

void test_pins() {
    if(state & 0x01)
        gpio_output_set(GPIO_OUTPUT_PIN_SEL, 0x00, GPIO_OUTPUT_PIN_SEL, 0x00);
    else
        gpio_output_set(0x00, GPIO_OUTPUT_PIN_SEL, GPIO_OUTPUT_PIN_SEL, 0x00);
    state++;
}