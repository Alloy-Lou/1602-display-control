#include "lcd.h"
#include "custom_characters.h"
#include "modules/_modules.h"

/* Configure and customize here your displays! :p

Create your own displays in the modules/ folder ^w^
Include both a .c file and a .h file (with #include <lcd.h>)
An example is provided, a few extra tools are also provided
in src/lcd.h, feel free to take a look over there!
If needed, run `make modules/modules.h` to manually generate or update modules.h

REGISTRATION ORDER MATTERS:
    - Main screens play sequentially in the exact order they are registered.
    - Side screens are checked in registration order whenever multiple side
      screens become due at the same timestamp (e.g., on the hour at :00).
    - Note that a full main rotation is forced between two due side screen plays.
*/
void setup_screens(void) {
    /* MAIN SCHEDULE — register_main_screen(render_fn, duration_sec)
        - Continuous, clock-synced loop anchored to :00 of every minute.
        - duration_sec: how long (in seconds, > 0) the screen stays visible.
        - Although not mandatory, it's advised to keep the sum of all `duration_sec` 
        values a divisor of 60 (e.g., 10s, 15s, 20s, 30s, or 60s) so the cycle aligns 
        cleanly with every minute boundary without cutting the last screen short at :59
        since it resets every minute.
    */
    register_custom_char(DEGREE_CELCIUS);

    register_main_screen(main_display,   7);
    register_main_screen(sub_display_en, 3);

    /* SIDE SCHEDULE — register_side_screen(render_fn, times_per_hour)
        - Periodic interrupts anchored to :00 of every hour 
          (useful for longer, lower-priority content, especially 
          multi-page displays (like weather forecasts or complex system summaries) 
          that would bloat the 1-minute main rotation.)
        - times_per_hour: how many times per hour the screen triggers (1..3600).
        Use divisors of 60 for predictable intervals, for example:
            1  -> every 60 min (:00)
            2  -> every 30 min (:00, :30)
            4  -> every 15 min (:00, :15, :30, :45)
            6  -> every 10 min (:00, :10, :20, ...)
            12 -> every  5 min (:00, :05, :10, ...)
        - On startup, each side screen waits until its next exact clock boundary
          before triggering for the first time.
        - The daemon calls `render_fn(&done)` once per second until the function sets
          `*done = 1` on its final tick, then runs one full main cycle before
          allowing another side screen to interrupt.
    */
    register_side_screen(test_side_display, 12);
}