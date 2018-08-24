#include "morphing_digits.h"

uint8_t zero[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b00000000, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t one[] = {0b00000000, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b00000000, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t two[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                 0b00000001, 0b00000001, 0b01111110, 0b10000000, 0b10000000,
                 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b01111110};

uint8_t three[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b01111110, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t four[] = {0b00000000, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t five[] = {0b01111110, 0b10000000, 0b10000000, 0b10000000, 0b10000000,
                  0b10000000, 0b10000000, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t six[] = {0b01111110, 0b10000000, 0b10000000, 0b10000000, 0b10000000,
                 0b10000000, 0b10000000, 0b01111110, 0b10000001, 0b10000001,
                 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t seven[] = {0b01111110, 0b00000001, 0b00000001, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000000, 0b00000001, 0b00000001,
                   0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000};

uint8_t eight[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                   0b10000001, 0b10000001, 0b01111110, 0b10000001, 0b10000001,
                   0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b01111110};

uint8_t nine[] = {0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b10000001,
                  0b10000001, 0b10000001, 0b01111110, 0b00000001, 0b00000001,
                  0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b01111110};

uint8_t* digits[10];

void morph(uint8_t from, uint8_t to, uint8_t* animation_type) {
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

void get_single_animation_step(uint8_t from, uint8_t to, uint8_t step, uint8_t* holder) {
    uint8_t animation_type[15];
    uint8_t* arr = digits[from];
    uint8_t* target = digits[to];
    morph(from, to, animation_type);
    for(uint8_t i = 0; i < 15; i++) {
        switch(animation_type[i]) {
            case NOTHING:
                *holder = arr[i];
                break;
            case REMOVE_RIGHT:
                *holder = (arr[i] << step) | target[i];
                break;
            case REMOVE_LEFT:
                *holder = (arr[i] >> step) | target[i];
                break;
            case REMOVE_LINE:
                *holder = arr[i] >> step;
                break;
            case MOVE_RIGHT_TO_LEFT:
                *holder = arr[i] << step;
                break;
            case MOVE_LEFT_TO_RIGHT:
                *holder = arr[i] >> step;
                break;
            case ADD_RIGHT:
                *holder = (arr[i] >> step) | arr[i];
                break;
            case ADD_LEFT:
                *holder = (arr[i] << step) | arr[i];
                break;
            case ADD_LINE:
                *holder = 0b01111110 >> (7 - step);
        }
        *holder = flip_bits(*holder);
        holder++;
    }
}

uint8_t flip_bits(uint8_t byte_to_flip) {
    uint8_t temp = 0;
    for(uint8_t j = 0; j < 8; j++) {
        temp |= (((byte_to_flip & (1<<j)) >> j) & 0x01) << (7-j);
    }
    return temp;
}

void get_digit(uint8_t digit, uint8_t* holder) {
    for(uint8_t i = 0; i < 15; i++) {
        *holder++ = flip_bits(digits[digit][i]);
    }
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
}