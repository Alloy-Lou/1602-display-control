#ifndef LCD_H
#define LCD_H

#include <time.h>

/**
 * Represents the two 16-character rows of the 1602 LCD.
 *
 * Strings longer than 16 visible characters are truncated when sent to the screen;
 * shorter strings are automatically right-padded with spaces.
**/
typedef struct {
    char line1[32];                         /**< Top row text (max 16 visible characters). */
    char line2[32];                         /**< Bottom row text (max 16 visible characters). */    
} display_string;

/**
 * Entry in the primary clock-synced rotation.
**/
typedef struct {
    display_string (*render)(void);         /**< Function returning the text to display. */
    int duration_sec;                       /**< Display duration in seconds (> 0). */
} main_screen_slot;

/**
 * Entry in the hourly-triggered side rotation.
**/
typedef struct {
    display_string (*render)(int *done);
    int times_per_hour;                     /**< Triggers per hour anchored to :00 (e.g., 1, 2, 4, 6, 12). */
    time_t last_slot;                       /**< Internal state tracker; always initialize to -1. */
} side_screen_slot;

// -----

/**
 * Registers the primary display rotation and automatically appends { NULL, 0 }.
 *
 * Example:
 * @code
 * REGISTER_MAIN_SCHEDULE(
 *     { main_display, 7 },
 *     { sub_display,  3 }
 * );
 * @endcode
**/
#define REGISTER_MAIN_SCHEDULE(...)                                                     \
    static main_screen_slot _main_schedule_data[] = { __VA_ARGS__ __VA_OPT__(,) { NULL, 0 } }; \
    main_screen_slot *main_schedule = _main_schedule_data

/**
 * Registers hourly side displays and automatically appends { NULL, 0, -1 }.
 *
 * Example:
 * @code
 * REGISTER_SIDE_SCHEDULE(
 *     { weather_display, 4, -1 },
 *     { quote_of_the_day_display,   2, -1 }
 * );
 * @endcode
**/
#define REGISTER_SIDE_SCHEDULE(...)                                                         \
    static side_screen_slot _side_schedule_data[] = { __VA_ARGS__ __VA_OPT__(,) { NULL, 0, -1 } }; \
    side_screen_slot *side_schedule = _side_schedule_data

// -----

/** Returns the current CPU package temperature in degrees Celsius. Rounded one digit after the comma (ex: 42.5) */
float get_cpu_temp(void);

/** Returns CPU usage percentage (0-100) since the previous 1-second tick. */
int get_cpu_usage(void);

/** Returns current RAM usage percentage (0-100). */
int get_ram_usage(void);

/**
 * Default main display layout:
 * Row 1: Date (DD/MM) | Time  
 * Row 2: System Temp (°C) | CPU% RAM%
 */
display_string main_display(void);

/**
 * Default sub display layout - French version:
 * Row 1: Weekday | Month Day  
 * Row 2: Month | Year
 */
display_string sub_display_fr(void);

/**
 * Test side display for debug/testing purposes;
 * Displays for 5 seconds:
 * Row 1: 1234567890123456  
 * Row 2: 2234567890123456
 */
display_string test_side_display(int *done);

/* Schedules defined externally in screens.c */
extern main_screen_slot main_schedule[];
extern side_screen_slot side_schedule[];

// -----

#endif