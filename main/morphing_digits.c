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

void morph(uint8_t from, uint8_t to, uint8_t** animation) {
    uint8_t* arr = digits[from];
    uint8_t* target = digits[to];

    //arr = zero;
    //target = one;

    uint8_t res[15];
    for(uint8_t i = 0; i < 15; i++) {
        //if the line in arr and target is the same, do nothing
        if(!(arr[i] ^ target[i]))
            res[i] = NOTHING;
        else if(arr[i] == 0b10000001 && target[i] == 0b00000001)
            res[i] = REMOVE_LEFT;
        else if(arr[i] == 0b10000001 && target[i] == 0b10000000)
            res[i] = REMOVE_RIGHT;
        else if(arr[i] == 0b00000001 && target[i] == 0b10000000)
            res[i] = MOVE_RIGHT_TO_LEFT;
        else if(arr[i] == 0b00000001 && target[i] == 0b10000001)
            res[i] = ADD_LEFT;
        else if(arr[i] == 0b10000000 && target[i] == 0b00000001)
            res[i] = MOVE_LEFT_TO_RIGHT;
        else if(arr[i] == 0b10000000 && target[i] == 0b10000001)
            res[i] = ADD_RIGHT;
        else if(arr[i] == 0b01111110 && target[i] == 0b00000000)
            res[i] = REMOVE_LINE;
        else if(arr[i] == 0b00000000 && target[i] == 0b01111110)
            res[i] = ADD_LINE;
    }

    for(uint8_t j = 0; j < 8; j++) {
        uint8_t out[15];
        for(uint8_t i = 0; i < 15; i++) {
            uint8_t action = res[i];
            switch(action) {
                case NOTHING:
                    out[i] = arr[i];
                    break;
                case REMOVE_RIGHT:
                    out[i] = (arr[i] << j) | target[i];
                    break;
                case REMOVE_LEFT:
                    out[i] = (arr[i] >> j) | target[i];
                    break;
                case REMOVE_LINE:
                    out[i] = arr[i] >> j;
                    break;
                case MOVE_RIGHT_TO_LEFT:
                    out[i] = arr[i] << j;
                    break;
                case MOVE_LEFT_TO_RIGHT:
                    out[i] = arr[i] >> j;
                    break;
                case ADD_RIGHT:
                    out[i] = (arr[i] >> j) | arr[i];
                    break;
                case ADD_LEFT:
                    out[i] = (arr[i] << j) | arr[i];
                    break;
                case ADD_LINE:
                    out[i] = 0b01111110 >> (7 - j);
            }
        }
        for(uint8_t a = 0; a < 15; a++) {
            animation[j][a] = out[a];
        }
    }
}

void get_digit(uint8_t digit, uint8_t* holder) {
    for(uint8_t i = 0; i < 15; i++) {
        *holder++ = digits[digit][i];
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