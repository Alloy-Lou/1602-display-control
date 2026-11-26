/*#include "lcd.h"

#include <stdint.h>
#include <uchar.h>

typedef struct {
    uint8_t rows[8];
    char32_t name;
} custom_char;

typedef struct {
    custom_char slots[8];
    uint8_t used_slots;
} custom_char_slots;

static custom_char_slots current_custom_char_slots = {
    {}, 0
};

int register_custom_char(custom_char c) {
    if (current_custom_char_slots.used_slots >= 8) return -1;

    // do the actual hardware registering here
    current_custom_char_slots.slots[current_custom_char_slots.used_slots] = c;
    current_custom_char_slots.used_slots++;
    return 0;
} */

#include "lcd.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static custom_char slots[8];
// There's 8 CGRAM slots to write custom characters, each bit signals if its corresponding slot is used
static uint8_t used_mask = 0b00000000;

int8_t register_custom_char(custom_char c) {
    for (uint8_t i = 0; i < 8; i++) {
        // (1u << i) builds a single bit mask to isolate the i bit on used_mask  
        // Retrieve if the n°i bit is free using the bit-wise AND (&)
        bool is_slot_used = used_mask & (1u << i);
        if (is_slot_used && memcmp(slots[i].rows, c.rows, 8) == 0) {
            // Reuse an existing slot if the exact 5x8 pixel bitmap is already loaded
            return (int8_t)(0x08 | i);
        } else if (!is_slot_used) {
            lcd_create_char(i, c);
            slots[i] = c;
            // | used as bitwise addition
            used_mask |= (uint8_t)(1u << i);
            return 0x08 | i;
        }
    }
    return -1; // All 8 CGRAM slots are occupied
}

int unregister_custom_char(int8_t char_code) {
    if (char_code < 0x08 || char_code > 0x0F) return -1;

    /* basically just does `char_code - 0x08` lol, just straight aura farming
        xxxx
      & 0111
      -------
        0xxx
    */
    uint8_t slot = (uint8_t)(char_code & 0x07);
    // 1u = 0000 0001 ; (1u << slot) is the slot we're freeing ; ~ flips everything ; & bitwise AND
    used_mask &= (uint8_t)~(1u << slot);
    return 0;
}

void free_custom_chars(void) {
    used_mask = 0b00000000;
}

const char *get_custom_char_name(uint8_t char_code) {
    if (char_code >= 0x08 && char_code <= 0x0F) {
        uint8_t slot = char_code & 0x07;
        if ((used_mask & (1u << slot)) && slots[slot].name[0] != '\0') {
            return slots[slot].name;
        }
    }
    return "?";
}

uint32_t utf_to_unicode(const char *s, int *idx) {
    // holy shii am not gonna pretend i'm smart enough to have written ts
    // ai slop explanation in dontreadme.c (pls forgive me for this sin)
    uint8_t b0 = (uint8_t)s[*idx];
    if (b0 < 0x80) {
        (*idx) += 1;
        return b0;
    }
    if ((b0 & 0xE0) == 0xC0 && ((uint8_t)s[*idx + 1] & 0xC0) == 0x80) {
        uint32_t charp = ((uint32_t)(b0 & 0x1F) << 6) | ((uint8_t)s[*idx + 1] & 0x3F);
        (*idx) += 2;
        return charp;
    }
    if ((b0 & 0xF0) == 0xE0 &&
        ((uint8_t)s[*idx + 1] & 0xC0) == 0x80 &&
        ((uint8_t)s[*idx + 2] & 0xC0) == 0x80) {
        uint32_t charp = ((uint32_t)(b0 & 0x0F) << 12) |
                      ((uint32_t)((uint8_t)s[*idx + 1] & 0x3F) << 6) |
                      ((uint8_t)s[*idx + 2] & 0x3F);
        (*idx) += 3;
        return charp;
    }
    if ((b0 & 0xF8) == 0xF0 &&
        ((uint8_t)s[*idx + 1] & 0xC0) == 0x80 &&
        ((uint8_t)s[*idx + 2] & 0xC0) == 0x80 &&
        ((uint8_t)s[*idx + 3] & 0xC0) == 0x80) {
        uint32_t charp = ((uint32_t)(b0 & 0x07) << 18) |
                      ((uint32_t)((uint8_t)s[*idx + 1] & 0x3F) << 12) |
                      ((uint32_t)((uint8_t)s[*idx + 2] & 0x3F) << 6) |
                      ((uint8_t)s[*idx + 3] & 0x3F);
        (*idx) += 4;
        return charp;
    }
    (*idx) += 1;
    return '?';
}

uint8_t unicode_to_rom(uint32_t unicode) {
    // Check if the character is already formatted as a custom character CGRAM address slot
    if (unicode >= 0x08 && unicode <= 0x0F) return (uint8_t)unicode;

    // Check if the character is a registered custom character
    for (uint8_t i = 0; i < 8; i++) {
        if ((used_mask & (1u << i)) && slots[i].name[0] != '\0') { // Skips to return '?' if registered without a name
            int idx = 0;
            if (utf_to_unicode(slots[i].name, &idx) == unicode) {
                return 0x08 | i;
            }
        }
    }

    // Standard ASCII
    if (unicode < 0x80) return (uint8_t)unicode;                

    switch (unicode) {
        /* Characters that are different between ASCII and the screen's ROM table 
        ** (at least mine, change this if it doesn't match on your part!!) 
        ** Additionnal sources: 
         - https://en.wikipedia.org/wiki/JIS_X_0201#Code_page_layout
         - https://en.wikipedia.org/wiki/HD44780_(integrated_circuit)#Font
             -> specifically this: "The 7-bit ASCII subset for the Japanese version is non-standard: 
                it supplies a Yen symbol where the backslash character is normally found, 
                and left and right arrow symbols in place of tilde and the rubout character."
         - https://en.wikipedia.org/wiki/JIS_X_0201#/media/File:Charset.gif
        */
        case 0x00A5: return 0x5C; // ¥
        case 0x2192: return 0x7E; // →
        case 0x2190: return 0x7F; // ←

        case 0x00B0: return 0xDF; // °

        case 0x03B1: return 0xE0; // α
        case 0x00E4: return 0xE1; // ä
        case 0x03B2: return 0xE2; // β
        case 0x03B5: return 0xE3; // ε
        case 0x00B5:              // µ (micro sign)
        case 0x03BC: return 0xE4; // μ (Greek small mu)
        case 0x03C3: return 0xE5; // σ
        case 0x03C1: return 0xE6; // ρ
        // tf is that, 'g'?
        case 0x221A: return 0xE8; // √
        // 0xE9: '⁻¹' (2 code points)
        // tf is that, 'j'?
        case 0x02E3: return 0xEB; // ˣ
        case 0x00A2: return 0xEC; // ¢
        case 0x00A3: return 0xED; // weird ass looking £
        case 0x00F1: return 0xEE; // ñ
        case 0x00F6: return 0xEF; // ö

        // tf is that, 'p'?
        // tf is that, 'q'?
        case 0x03B8: return 0xF2; // θ
        case 0x221E: return 0xF3; // ∞
        case 0x03A9:              // Ω (Greek capital omega)
        case 0x2126: return 0xF4; // Ω (ohm sign)
        case 0x00FC: return 0xF5; // ü
        case 0x03A3: return 0xF6; // Σ
        case 0x03C0: return 0xF7; // π
        // 0xF8: 'x̄' (2 code points)
        // tf is that, 'y'?
        case 0x5343: return 0xFA; // 千 good thing i recognized those chinese characters :pp
        case 0x4E07: return 0xFB; // 万
        case 0x5186: return 0xFC; // 円
        case 0x00F7: return 0xFD; // ÷
        // 0xFE: blank
        case 0x2588: return 0xFF; // █

        default:     return '?';
    }
}