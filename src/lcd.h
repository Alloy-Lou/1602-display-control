#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <time.h>

/**
 * Represents the two 16-character rows of the 1602 LCD.
 *
 * Strings longer than 16 visible characters are truncated when sent to the screen;
 * shorter strings are automatically right-padded with spaces.
**/
typedef struct {
    /** Top row text (max 16 visible characters). Size 4*16=64 to accomodate for 4-byte UTF-8 custom characters. */
    char line1[64];
    /** Bottom row text (max 16 visible characters). Size 4*16=64 to accomodate for 4-byte UTF-8 custom characters. */  
    char line2[64];  
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

typedef struct {
    /** 
        Array representing your custom character. Characters are being displayed on 5x8 pixels which means
        6th, 7th and 8th bits of each row will be ignored.
        Use https://esp32.implrust.com/lcd/lcd-custom-char-generator.html or a similar tool to design your characters.
    */
    uint8_t rows[8];   
    /** 
        Up to 4-byte UTF-8 character ("é", "℃", "☀") + '\0'  
        This character will be written to SCREEN_FILE (/dev/shm/lcd.screen) and will appear when using `make monitor`
    */                     
    char name[5];
} custom_char;

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
**/
display_string main_display(void);

/**
 * Default sub display layout - French version:
 * Row 1: Weekday | Month Day  
 * Row 2: Month | Year
**/
display_string sub_display_fr(void);

/**
 * Default sub display layout - English version:
 * Row 1: Weekday | Month Day  
 * Row 2: Month | Year
**/
display_string sub_display_en(void);

// -----

/**
 * Loads a custom character into CGRAM (or reuses its slot if already loaded).
 * 1602 screens (at least mine) has 8 writable-to-RAM custom characters slots,
 * located from 0x00 to 0x07, mirrored from 0x08 to 0x0F
 * Maps all 8 slots to the hardware mirror codes so strings never contain 0x00 ('\0').
 *
 * Returns the address slot (0x08 to 0x0F) on success, or -1 if all 8 slots are full.
 *
 * WARNING : Do not forget to free those slots so other modules/screens can register theirs!
 * You can re-use the same slot within the same module, but you can't use more than
 * 8 custom characters at once (hardware limitation)
**/
int8_t register_custom_char(custom_char c);

/**
 * Frees a single slot using the custom character's address code (0x08 to 0x0F) returned by
 * register_custom_char(). Returns 0 on success, or -1 if the code is invalid.
**/
int unregister_custom_char(int8_t char_code);

/**
 * Frees all 8 custom character slots at once.
**/
void free_custom_chars(void);

/**
 * Returns the custom character's name using the custom character's address code (0x08 to 0x0F)
 * Used when rendering /dev/shm/lcd.screen.
 * Returns '?' if the character is invalid.
**/
const char* get_custom_char_name(uint8_t char_code);

/* -----
** The following functions are mainly meant to be used in main.c; you shouldn't need them
----- */

/**
 * Loads a custom character into CGRAM (this function actually does the hardware part)
 * Avoid using this function, please use register_custom_char() instead.
**/
void lcd_create_char(uint8_t slot, const custom_char c);

/**
 * Reads one complete UTF-8 character from a C string starting at index *idx, 
 * converts its 1 to 4 bytes into a single Unicode ID number (which is returned) 
 * and advances *idx to the first byte of the next character.
**/
uint32_t utf_to_unicode(const char *s, int *idx);

/**
 * Translates the provided Unicode into the equivalent character from the LCD's character table.
 * This function do handle custom characters as well. If no matching character can be found, returns '?'
**/
uint8_t unicode_to_rom(uint32_t unicode);

// -----

/**
 * Register all your main and side displays here, 
 * using register_main_screen() and register_side_screen()
**/
void setup_screens(void);

#endif