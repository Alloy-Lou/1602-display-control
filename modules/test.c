#include "test.h"

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