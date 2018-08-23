#include "rgb_matrix.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>
#include "freertos/event_groups.h"

#define LED_TAG "LED_MATRIX"

#define UPDATED_BIT BIT0

static uint32_t buffer[BUFFER_LENGTH * 2];
uint32_t* buffer_ptr = buffer + BUFFER_LENGTH;
uint32_t* shared_buffer_ptr = buffer;
SemaphoreHandle_t buffer_semaphore;
SemaphoreHandle_t buffer_swap_semaphore;
uint8_t state = 0;
uint8_t brightness = 100;

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
}

void update_display() {
    if(xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE) {
        if(xSemaphoreTake(buffer_swap_semaphore, (TickType_t) 10) == pdTRUE) {
            uint32_t* temp = buffer_ptr;
            buffer_ptr = shared_buffer_ptr;
            shared_buffer_ptr = temp;
            xSemaphoreGive(buffer_swap_semaphore);
        } else {
            ESP_LOGE(LED_TAG, "Failed to swap the buffers");
        }
        xSemaphoreGive(buffer_semaphore);
        clear_display();
    }
}

void draw_display() {
    if(xSemaphoreTake(buffer_swap_semaphore, (TickType_t) 10) == pdTRUE) {
        uint8_t pos, r0_val, b0_val, g0_val, r1_val, g1_val, b1_val;
        for(uint8_t i = 0; i < 16; i++) {
            gpio_set_level(A_pin, i & 0x01);
            gpio_set_level(B_pin, (i & 0x02) >> 1);
            gpio_set_level(C_pin, (i & 0x04) >> 2);
            gpio_set_level(D_pin, (i & 0x08) >> 3);
            for(uint8_t k = 0; k < 64; k++) {
                    pos = i*2 + (k/32);
                    r0_val = (buffer_ptr[pos + R0_OFFSET] >> (k%32)) & 0x01;
                    g0_val = (buffer_ptr[pos + G0_OFFSET] >> (k%32)) & 0x01;
                    b0_val = (buffer_ptr[pos + B0_OFFSET] >> (k%32)) & 0x01;
                    r1_val = (buffer_ptr[pos + R1_OFFSET] >> (k%32)) & 0x01;
                    g1_val = (buffer_ptr[pos + G1_OFFSET] >> (k%32)) & 0x01;
                    b1_val = (buffer_ptr[pos + B1_OFFSET] >> (k%32)) & 0x01;
                    
                    gpio_output_set((r0_val<<r0_pin) | (g0_val<<g0_pin) | (b0_val<<b0_pin) | \
                                    (r1_val<<r1_pin) | (g1_val<<g1_pin) | (b1_val<<b1_pin) , \
                                    ((1-r0_val)<<r0_pin) | ((1-g0_val)<<g0_pin) | ((1-b0_val)<<b0_pin) | \
                                    ((1-r1_val)<<r1_pin) | ((1-g1_val)<<g1_pin) | ((1-b1_val)<<b1_pin), \
                                    (1<<r0_pin) | (1<<g0_pin) | (1<<b0_pin) | \
                                    (1<<r1_pin) | (1<<g1_pin) | (1<<b1_pin), 0x00);
                    gpio_set_level(CLK, 1);
                    gpio_set_level(CLK, 0);
            }
            xSemaphoreGive(buffer_swap_semaphore);
            gpio_set_level(LAT, 1);
            gpio_set_level(LAT, 0);
            gpio_set_level(OE, 0);
            for(uint16_t j = 0; j < LED_ON_TIME * brightness; j++) { //TODO: fix real delay TODO: brightness is not thread safe
                asm volatile ("NOP;");
            }
            gpio_set_level(OE, 1);
        }
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
    if(x%32 < 32-8) {
        uint32_t digit_r[15];
        uint32_t digit_g[15];
        uint32_t digit_b[15];
        for(uint8_t h = 0; h < 15; h++) {
            digit_r[h] = (r[h] << ((x%32)));
            digit_g[h] = (g[h] << ((x%32)));
            digit_b[h] = (b[h] << ((x%32)));
        }
        if(buffer_semaphore != NULL) {
            if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
                for(uint8_t h = 0; h < 15; h++) {
                    shared_buffer_ptr[((y + h) * 2) + (x/32) + R0_OFFSET] = digit_r[h];
                    shared_buffer_ptr[((y + h) * 2) + (x/32) + G0_OFFSET] = digit_g[h];
                    shared_buffer_ptr[((y + h) * 2) + (x/32) + B0_OFFSET] = digit_b[h];
                }
                xSemaphoreGive(buffer_semaphore);
            }
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
            brightness = brightness_val;
            if(brightness > 100)
                brightness = 100;
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