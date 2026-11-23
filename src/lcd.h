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
 * Registers the primary display rotation. Order of calls matters
 * Returns 0 on success, -1 on error.
 *
 * Example:
 * register_main_screen(main_display,   7);
 * register_main_screen(sub_display_fr, 3);
 * -> will register on a 10 sec rotation "main_display" for 7s, then "sub_display_fr" for 3s
**/
int register_main_screen(display_string (*render)(void), int duration_sec);

/**
 * Registers hourly side displays
 * Returns 0 on success, -1 on error.
 *
 * Example:
 * register_side_screen(test_side_display, 6);
 * -> will register test_side_display 6 times an hour, so every 10 minutes
**/
int register_side_screen(display_string (*render)(int *done), int times_per_hour);

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
 * Default sub display layout - English version:
 * Row 1: Weekday | Month Day  
 * Row 2: Month | Year
 */
display_string sub_display_en(void);

// -----

/**
 * Register all your main and side displays here, 
 * using register_main_screen() and register_side_screen()
 * 
**/
void setup_screens(void);

#endif