#include "rgb_matrix.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>

#define LED_TAG "LED_MATRIX"

static uint32_t buffer[BUFFER_LENGTH * 2];
uint32_t* buffer_ptr = buffer + BUFFER_LENGTH;
uint32_t* shared_buffer_ptr = buffer;
SemaphoreHandle_t buffer_semaphore;
uint8_t state = 0;

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
}

void update_display() {
    //ESP_LOGI(LED_TAG, "UPDATE DISPLAY");
    // swap the buffer, if not available draw the old one
    if(xSemaphoreTake(buffer_semaphore, (TickType_t) 5) == pdTRUE) {
        uint32_t* temp = buffer_ptr;
        buffer_ptr = shared_buffer_ptr;
        shared_buffer_ptr = temp;        
        xSemaphoreGive(buffer_semaphore);
    }
    for(uint8_t i = 0; i < 16; i++) {
        gpio_set_level(A_pin, i & 0x01);
        gpio_set_level(B_pin, (i & 0x02) >> 1);
        gpio_set_level(C_pin, (i & 0x04) >> 2);
        gpio_set_level(D_pin, (i & 0x08) >> 3);
        for(uint8_t k = 0; k < 64; k++) {
                gpio_set_level(r0_pin, (buffer_ptr[i*2 + R0_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(g0_pin, (buffer_ptr[i*2 + G0_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(b0_pin, (buffer_ptr[i*2 + B0_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(r1_pin, (buffer_ptr[i*2 + R1_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(g1_pin, (buffer_ptr[i*2 + G1_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(b1_pin, (buffer_ptr[i*2 + B1_OFFSET + (k/32)] >> (k%32) & 0x01));
                gpio_set_level(CLK, 1);
                gpio_set_level(CLK, 0);
        }
        gpio_set_level(LAT, 1);
        gpio_set_level(LAT, 0);
        gpio_set_level(OE, 0);
        for(uint8_t j = 0; j < LED_ON_TIME; j++) { //TODO fix real delay
            for(uint32_t k = 0; k < 100; k++) {
                asm volatile ("NOP;");
            }
        }
        gpio_set_level(OE, 1);
    }
}

void set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    // error check if the pixel is outside of panel, since it's unsigned integer, most unsigned values will be outside (iffy)
    if(x >= WIDTH || y >= HEIGHT)
        return;
    //ESP_LOGI(LED_TAG, "setting x:%d, y:%d\t%d %d %d", x, y, r, g, b);

    uint8_t pos = (y * 2) + (x / 32);
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

void set_multiple_pixels(uint8_t x, uint8_t y, uint8_t* r, uint8_t* g, uint8_t* b, size_t x_size, size_t y_size) {
    for(uint8_t i = 0; i < y_size; i++) {
        for(uint8_t j = 0; j < x_size; j++) {
            set_pixel(x + j, y + i, r[i*x_size + j], g[i*x_size + j], b[i*x_size + j]);
        }
    }
}

void clear_display() {
    if((xSemaphoreTake(buffer_semaphore, (TickType_t) 10) == pdTRUE)) {
        memset(shared_buffer_ptr, 0, BUFFER_LENGTH);
    }
}

void test_pins() {
    if(state & 0x01)
        gpio_output_set(GPIO_OUTPUT_PIN_SEL, 0x00, GPIO_OUTPUT_PIN_SEL, 0x00);
    else
        gpio_output_set(0x00, GPIO_OUTPUT_PIN_SEL, GPIO_OUTPUT_PIN_SEL, 0x00);
    state++;
}