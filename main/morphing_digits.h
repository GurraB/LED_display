#ifndef morphing_digits_h
#define morphing_digits_h
#include <inttypes.h>
#include "rgb_matrix.h"

#define NOTHING 0
#define REMOVE_RIGHT 1
#define REMOVE_LEFT 2
#define REMOVE_LINE 3
#define MOVE_RIGHT_TO_LEFT 4
#define MOVE_LEFT_TO_RIGHT 5
#define ADD_RIGHT 6
#define ADD_LEFT 7
#define ADD_LINE 8

uint8_t flip_bits(uint8_t byte_to_flip);

void get_digit(uint8_t digit, struct rgb_color* holder, struct rgb_color color);

void get_single_animation_step(uint8_t from, uint8_t to, uint8_t step, struct rgb_color *holder, struct rgb_color color);

void init_morphing_digits();

#endif