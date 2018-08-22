#ifndef rgb_matrix_h
#define rgb_matrix_h
#include <inttypes.h>

// Display width and height
#define WIDTH 64
#define HEIGHT 32

//It is possible to get more than one bit color depth, however I will implement it later on
#define COLOR_DEPTH 1

/*
 * The length of the buffer, each pixel takes one bit and using uint32_t buffer
 * results in a buffer size:
 * (number of pixels / pixels per element in array) * height * number of colors * color depth
 * This might not work for other displays
*/
#define BUFFER_LENGTH (WIDTH / 32) * HEIGHT * 3 * COLOR_DEPTH

// Each color pin covers half the display
#define SINGLE_COLOR_BUFFER_LENGTH ((WIDTH / 32) * (HEIGHT / 2))
#define R0_OFFSET 0
#define R1_OFFSET SINGLE_COLOR_BUFFER_LENGTH
#define G0_OFFSET SINGLE_COLOR_BUFFER_LENGTH * 2
#define G1_OFFSET SINGLE_COLOR_BUFFER_LENGTH * 3
#define B0_OFFSET SINGLE_COLOR_BUFFER_LENGTH * 4
#define B1_OFFSET SINGLE_COLOR_BUFFER_LENGTH * 5

// The time the pixels are ON in microseconds
#define LED_ON_TIME 70

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

void init_rgb_matrix(void);

void update_display(void);

void set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);

void test_pins(void);

#endif