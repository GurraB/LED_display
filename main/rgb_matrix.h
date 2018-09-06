#ifndef rgb_matrix_h
#define rgb_matrix_h
#include <inttypes.h>
#include <stdlib.h>

// Display width and height
#define WIDTH 64
#define HEIGHT 32

/*
 * The length of the buffer, each pixel takes one byte and using uint8_t buffer
 * results in a buffer size:
 * width * height * number of colors
 * It is not the most memory efficient, however it is simple and the ESP32 has something like 512KB of SRAM anyways.
*/
#define BUFFER_LENGTH (WIDTH * HEIGHT * 3)

// Each color pin covers half the display
#define SINGLE_COLOR_BUFFER_LENGTH ((WIDTH * HEIGHT) / 2)
#define RED 0
#define GREEN 1
#define BLUE 2

// The time the pixels are ON in microseconds at 1% brightness
#define LED_ON_TIME 1

// The pins used to send the pixel data
#define r0_pin 25
#define g0_pin 26
#define b0_pin 27
#define r1_pin 14
#define g1_pin 12
#define b1_pin 13

// The pins used to select which row to send data to and show
#define A_pin 23
#define B_pin 22
#define C_pin 21
#define D_pin 19

// The pin used to supply clock signal to the data pins
#define CLK 5
// TODO: explain
#define LAT 17
// The pin that enables the display, i.e pixels on/off
#define OE 16

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<r0_pin) | (1ULL<<g0_pin) | (1ULL<<b0_pin) | \
                            (1ULL<<r1_pin) | (1ULL<<g1_pin) | (1ULL<<b1_pin) | \
                            (1ULL<<A_pin) | (1ULL<<B_pin) | (1ULL<<C_pin) | (1ULL<<D_pin) | \
                            (1ULL<<CLK) | (1ULL<<LAT) | (1ULL<<OE))

struct rgb_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void init_rgb_matrix(void);

void update_display(void);

void draw_display(void);

void set_pixel(uint8_t x, uint8_t y, struct rgb_color color);

void set_pixel_in_buffer(uint8_t x, uint8_t y, struct rgb_color color, uint8_t* buff);

void set_display(uint8_t* buff);

void clear_display(void);

void set_brightness(uint8_t brightness_val);

void draw_digit(uint8_t x, uint8_t y, struct rgb_color* digit);

struct rgb_color get_color(uint8_t r, uint8_t g, uint8_t b);

#endif