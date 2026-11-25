#include "test.h"

#include <stdint.h>

display_string test_side_display(int* done) {
    static int t = 0;
    display_string test = {"1234567890123456", "2234567890123456"};
    if (t >= 5) {
        t = -1; // t++ right after. ew code ik but "flemme" okay
        *done = 1;
    }

    t++;
    return test;
}

display_string list_characters(int *done) {
    static int tick = 0;
    display_string out = {0};

    uint8_t base = (uint8_t)((tick / 5) * 32); // Advances by 32 chars every 5s

    for (int i = 0; i < 16; i++) {
        uint8_t c1 = (uint8_t)(base + i);
        uint8_t c2 = (uint8_t)(base + 16 + i);

        // Only 0x00 must be blanked so snprintf() in lcd_write_line() doesn't
        // truncate row 1. Custom slots 1..7 (0x01..0x07) and mirrored slots
        // 0..7 (0x08..0x0F) will all render normally.
        out.line1[i] = (c1 == 0x00) ? ' ' : (char)c1;
        out.line2[i] = (c2 == 0x00) ? ' ' : (char)c2;
    }
    out.line1[16] = '\0';
    out.line2[16] = '\0';

    tick++;
    if (tick >= 40) { // 8 pages * 5 seconds = 40 ticks
        tick = 0;
        *done = 1;
    }

    return out;
}