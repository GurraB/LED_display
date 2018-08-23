#ifndef morphing_digits_h
#define morphing_digits_h
#include <inttypes.h>

#define NOTHING 0
#define REMOVE_RIGHT 1
#define REMOVE_LEFT 2
#define REMOVE_LINE 3
#define MOVE_RIGHT_TO_LEFT 4
#define MOVE_LEFT_TO_RIGHT 5
#define ADD_RIGHT 6
#define ADD_LEFT 7
#define ADD_LINE 8

void morph(uint8_t from, uint8_t to, uint8_t** animation);

void get_digit(uint8_t digit, uint8_t* holder);

void init_morphing_digits();

#endif