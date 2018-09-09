#include "morphing_digits.h"
#include "rgb_matrix.h"
#include <string.h>

uint8_t zero[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b00000000, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t zero_small[] = {0b00000110, 0b00001001, 0b00001001, 0b00000000,
                        0b00001001, 0b00001001, 0b00000110};


uint8_t one[] = {0b00000000, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b00000000, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t one_small[] = {0b00000000, 0b00000001, 0b00000001, 0b00000000,
                       0b00000001, 0b00000001, 0b00000000};

uint8_t two[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b01111110, 0b10000000, 0b10000000,
                 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b01111110};

uint8_t two_small[] = {0b00000110, 0b00000001, 0b00000001, 0b00000110,
                       0b00001000, 0b00001000, 0b00000110};

uint8_t three[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b01111110, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t three_small[] = {0b00000110, 0b00000001, 0b00000001, 0b00000110,
                         0b00000001, 0b00000001, 0b00000110};

uint8_t four[] = {0b00000000, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t four_small[] = {0b00000000, 0b00001001, 0b00001001, 0b00000110,
                        0b00000001, 0b00000001, 0b00000000};

uint8_t five[] = {0b01111110, 0b10000000, 0b10000000, 0b10000000, 0b10000000,
                  0b10000000, 0b10000000, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t five_small[] = {0b00000110, 0b00001000, 0b00001000, 0b00000110,
                        0b00000001, 0b00000001, 0b00000110};

uint8_t six[] = {0b01111110, 0b10000000, 0b10000000, 0b10000000, 0b10000000,
                 0b10000000, 0b10000000, 0b01111110, 0b10000001, 0b10000001,
                 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t six_small[] = {0b00000110, 0b00001000, 0b00001000, 0b00000110,
                       0b00001001, 0b00001001, 0b00000110};

uint8_t seven[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000000, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t seven_small[] = {0b00000110, 0b00000001, 0b00000001, 0b00000000,
                         0b00000001, 0b00000001, 0b00000000};

uint8_t eight[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                   0b10000001, 0b10000001, 0b01111110, 0b10000001, 0b10000001,
                   0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t eight_small[] = {0b00000110, 0b00001001, 0b00001001, 0b00000110,
                         0b00001001, 0b00001001, 0b00000110};

uint8_t nine[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t nine_small[] = {0b00000110, 0b00001001, 0b00001001, 0b00000110,
                        0b00000001, 0b00000001, 0b00000110};

uint8_t* digits[10];
uint8_t* digits_small[10];

struct rgb_color black = {
    .r = 0,
    .g = 0,
    .b = 0
};

void morph(uint8_t from, uint8_t to, uint8_t* animation_type, uint8_t digit_size) {
    if(digit_size == SMALL) {
        uint8_t* arr = digits_small[from];
        uint8_t* target = digits_small[to];

        for(uint8_t i = 0; i < 7 ; i++) {
            //if the line in arr and target is the same, do nothing
            if(!(arr[i] ^ target[i]))
                *animation_type++ = NOTHING;
            else if(arr[i] == 0b00001001 && target[i] == 0b00000001)
                *animation_type++ = REMOVE_LEFT;
            else if(arr[i] == 0b00001001 && target[i] == 0b00001000)
                *animation_type++ = REMOVE_RIGHT;
            else if(arr[i] == 0b00000001 && target[i] == 0b00001000)
                *animation_type++ = MOVE_RIGHT_TO_LEFT;
            else if(arr[i] == 0b00000001 && target[i] == 0b00001001)
                *animation_type++ = ADD_LEFT;
            else if(arr[i] == 0b00001000 && target[i] == 0b00000001)
                *animation_type++ = MOVE_LEFT_TO_RIGHT;
            else if(arr[i] == 0b00001000 && target[i] == 0b00001001)
                *animation_type++ = ADD_RIGHT;
            else if(arr[i] == 0b00000110 && target[i] == 0b00000000)
                *animation_type++ = REMOVE_LINE;
            else if(arr[i] == 0b00000000 && target[i] == 0b00000110)
                *animation_type++ = ADD_LINE;
        }
    } else if (digit_size == REGULAR) {
        uint8_t* arr = digits[from];
        uint8_t* target = digits[to];

        for(uint8_t i = 0; i < 15; i++) {
            //if the line in arr and target is the same, do nothing
            if(!(arr[i] ^ target[i]))
                *animation_type++ = NOTHING;
            else if(arr[i] == 0b10000001 && target[i] == 0b00000001)
                *animation_type++ = REMOVE_LEFT;
            else if(arr[i] == 0b10000001 && target[i] == 0b10000000)
                *animation_type++ = REMOVE_RIGHT;
            else if(arr[i] == 0b00000001 && target[i] == 0b10000000)
                *animation_type++ = MOVE_RIGHT_TO_LEFT;
            else if(arr[i] == 0b00000001 && target[i] == 0b10000001)
                *animation_type++ = ADD_LEFT;
            else if(arr[i] == 0b10000000 && target[i] == 0b00000001)
                *animation_type++ = MOVE_LEFT_TO_RIGHT;
            else if(arr[i] == 0b10000000 && target[i] == 0b10000001)
                *animation_type++ = ADD_RIGHT;
            else if(arr[i] == 0b01111110 && target[i] == 0b00000000)
                *animation_type++ = REMOVE_LINE;
            else if(arr[i] == 0b00000000 && target[i] == 0b01111110)
                *animation_type++ = ADD_LINE;
        }
    }
}

uint8_t flip_bits(uint8_t byte_to_flip) {
    uint8_t temp = 0;
    for(uint8_t j = 0; j < 8; j++) {
        temp |= (((byte_to_flip & (1<<j)) >> j) & 0x01) << (7-j);
    }
    return temp;
}

void get_digit(uint8_t digit, struct rgb_color *holder, struct rgb_color color, uint8_t digit_size) {
    if(digit_size == SMALL) {
        uint8_t temp[7];
        for(uint8_t i = 0; i < 7; i++) {
            temp[i] = flip_bits(digits_small[digit][i]);
            temp[i] = temp[i] >> 4;
            for(uint8_t j = 0; j < 8; j++) {
                if((temp[i] >> j) & 0x01) {
                    *holder++ = color;
                } else {
                    *holder++ = black;
                }
            }
        }
    } else if(digit_size == REGULAR) {
        uint8_t temp[15];
        for(uint8_t i = 0; i < 15; i++) {
            temp[i] = flip_bits(digits[digit][i]);
            for(uint8_t j = 0; j < 8; j++) {
                if((temp[i] >> j) & 0x01) {
                    *holder++ = color;
                } else {
                    *holder++ = black;
                }
            }
        }
    }
}

void get_single_animation_step(uint8_t from, uint8_t to, uint8_t step, struct rgb_color *holder, struct rgb_color color, uint8_t digit_size) {
    uint8_t* temp = malloc(digit_size * sizeof(uint8_t));
    uint8_t* animation_type = malloc(digit_size * sizeof(uint8_t));
    uint8_t* arr;
    uint8_t* target;
    if(digit_size == SMALL) {
        arr = digits_small[from];
        target = digits_small[to];
    } else if(digit_size == REGULAR) {
        arr = digits[from];
        target = digits[to];
    } else { //default to REGULAR size
        arr = digits[from];
        target = digits[to];
    }
    morph(from, to, animation_type, digit_size);
    for(uint8_t i = 0; i < digit_size; i++) {
        switch(animation_type[i]) {
            case NOTHING:
                temp[i] = arr[i];
                break;
            case REMOVE_RIGHT:
                temp[i] = (arr[i] << step) | target[i];
                break;
            case REMOVE_LEFT:
                temp[i] = (arr[i] >> step) | target[i];
                break;
            case REMOVE_LINE:
                temp[i] = arr[i] >> step;
                break;
            case MOVE_RIGHT_TO_LEFT:
                temp[i] = arr[i] << step;
                break;
            case MOVE_LEFT_TO_RIGHT:
                temp[i] = arr[i] >> step;
                break;
            case ADD_RIGHT:
                temp[i] = (arr[i] >> step) | arr[i];
                break;
            case ADD_LEFT:
                temp[i] = (arr[i] << step) | arr[i];
                break;
            case ADD_LINE:
                if(digit_size == SMALL) {
                    temp[i] = 0b01100000 >> (7 - step);
                } else if(digit_size == REGULAR) {
                    temp[i] = 0b01111110 >> (7 - step);
                }
                break;
        }
        temp[i] = flip_bits(temp[i]);
        if(digit_size == SMALL) {
            temp[i] = temp[i] >> 4;
        }
        for(uint8_t j = 0; j < 8; j++) {
            if((temp[i] >> j) & 0x01) {
                *holder++ = color;
            } else {
                *holder++ = black;
            }
        }
    }
    free(temp);
    free(animation_type);
}

void init_morphing_digits() {
    digits[0] = zero;
    digits[1] = one;
    digits[2] = two;
    digits[3] = three;
    digits[4] = four;
    digits[5] = five;
    digits[6] = six;
    digits[7] = seven;
    digits[8] = eight;
    digits[9] = nine;
    
    digits_small[0] = zero_small;
    digits_small[1] = one_small;
    digits_small[2] = two_small;
    digits_small[3] = three_small;
    digits_small[4] = four_small;
    digits_small[5] = five_small;
    digits_small[6] = six_small;
    digits_small[7] = seven_small;
    digits_small[8] = eight_small;
    digits_small[9] = nine_small;
}