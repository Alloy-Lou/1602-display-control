#include "lcd.h"

void setup_screens(void) {
    register_main_screen(main_display,   7);
    register_main_screen(sub_display_fr, 3);

    register_side_screen(test_side_display, 24);
}