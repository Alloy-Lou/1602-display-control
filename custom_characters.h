#include "lcd.h"

/*
    Add your custom characters here!
    You can use a tool to like 
    https://esp32.implrust.com/lcd/lcd-custom-char-generator.html
    to design them.

    Please use the custom_char struct from lcd.h to export them:
    typedef struct {
        uint8_t rows[8];      
        char name[5];
    } custom_char;

    Declare them as `static const custom_char` so multiple files can include this header.
*/

static const custom_char DEGREE_CELCIUS = {
    .rows= {
        0b11000, // ████░░░░░░
        0b11000, // ████░░░░░░
        0b00000, // ░░░░░░░░░░
        0b00111, // ░░░░██████
        0b01000, // ░░██░░░░░░
        0b01000, // ░░██░░░░░░
        0b01000, // ░░██░░░░░░
        0b00111  // ░░░░██████
    },
    .name = "℃"
};