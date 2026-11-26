For the future me who'll have no fucking idea what any of this sh!t do

From what i understood: the lil black thingy listens for 8bit packets (octets) like this:
1234 5678

1-4 : DATA
5   : Backlight LED (1 = ON, 0 = OFF).
6   : Clock
7   : Read = 1, Write = 0
8   : "Mode" : 0 = command, 1 = ASCII

The lil black thingy takes characters as input, no need to write em pixel by pixel
however that means limited creativity/font :(
there's still 8 slots or whatever to create custom characters

the catch is a character is also 8bit (le octet for the frenchie me)
and we need to fit the character into a lil 4 bit data input (would be too easy otherwise haha ಥ‿ಥ)
sooooo we cut each character into 2x4 bit and feed the halfs (halfes? halves? idk how to spell)

eeeeexample timeeee!

"Proute"
 ^ P = 11010110 (it's not actually that idk but just imagine ok)

cut it *snip* *snip* -> 1101 | 0110

now the 4 other bits: 1001

1 : cause we wanna see shit
0 : le outil mystère qui nous servira plus tard
0 : we writiiin
1 : we givin characters (commands is for things like "place cursor at beningingning of the line)

so we wanna send those two packets:

1101 1001 (first half), 0110 1001 -> the lgbt understands we wanna write 'P' and draws it on the screen :D

if the future me hasn't ended completely brainrotted (i wanna die rn) you'll ask
"wait, but how does the lgbt ('lil great black thingy) know i've sent it something?"
and if the future me is really smart, will ask:
"if a character's halfs are similar, how does it know it did refresh and is looking at twice the same packet?"

remember the *outil mystère qui nous servira plus tard* ? that's what the clock is (mostly) for!
yea :p *some cool music idk*

so we wanna use the clock to sync everything in place, 'cause electricity isn't as perfect as me (haha jk)
and needs time to stabilize / the lgbt have time to read everything cause it technically has 8bit input
but only one input wire connected to the raspberry pi :/ (if im not mistaken)

so we actually send those 4 packets for one character, with a very lil delay

1101 1001 -> initialize sent data (data goes from 0000 to 1101 one bit after another i guess)
1101 1101 -> by now the LOW/HIGH electricity values should be correct and ready to be read by the lgbt
0110 1001 -> we force gender transition on the data bit values muehehe (1101 -> 0110)
0110 1101 -> woaw super fast and efficient hrt that turn 0s into 1s, and 1s into 0s called electricity (kinda)

the lgbt is now able to write the 'P' letter :p
start over for the 'r' now :)

btwww!
the bitwise operators! cause i'm not using that sh!t often in JS

& : bitwise AND (1 & 1 -> 1, otherwise 0)
| : ""      OR (0 | 0 -> 0, otherwise 1)
<< : we movin sh!t (no need to explain that one right?)

note that | can be used as a "bitwise addition" OwO

so the snip-snipping works like that:

  1101 1001     P
& 1111 0000     the snip-snipping
-----------
  1101 0000     P is now snip-snipped!

Turn a flag on:

  1101 1001
| 0000 0100
-----------
  1101 1101     clock is clocking >:p

yay

note from the future me but not the future one who will read ts:
for some fking reason the clock actually works the other way around: 1 first, and reads on 0

---

lcd_init codes used:

https://en.wikipedia.org/wiki/HD44780_(integrated_circuit)#Mode_selection

0x40 (00 01xx xxxx) -> Set CGRAM address (used for custom chars :>)

0x33, 0x32 -> init/hardware handshake:

warning: AI-slop below explaining ts to my needy brain:
Byte 1 (0x33) ->  Trip 1: 0011 ("Switch to 8-bit mode")
                  Trip 2: 0011 ("Switch to 8-bit mode")

Byte 2 (0x32) ->  Trip 3: 0011 ("Switch to 8-bit mode")
                  Trip 4: 0010 ("Now switch to 4-bit mode!")

Sending 0011 three times in a row is a universal hardware handshake that works no matter what state the screen was previously stuck in:

If the screen started in 8-bit mode: It receives "Switch to 8-bit mode" (0011) three times in a row (staying in 8-bit mode), and then the 4th pulse (0010) cleanly switches it into 4-bit mode.

If the screen was already in 4-bit mode: It pairs up the first two 0011s into one command (0011 0011) that switches it back to 8-bit mode. Then the 3rd pulse (0011) keeps it in 8-bit mode, and the 4th pulse (0010) cleanly switches it back into 4-bit mode.

If the screen was stuck halfway through a 4-bit byte: The 1st 0011 finishes whatever broken command it was waiting on. Then the 2nd and 3rd 0011s pair up (0011 0011) to force it into 8-bit mode, and the 4th pulse (0010) switches it cleanly into 4-bit mode.

Once that 4-step handshake finishes (0x33, 0x32), the screen is guaranteed to be synchronized in 4-bit mode—so it can now properly glue together the two halves of 0x28 (2 lines), 0x0C (display ON), 0x06 (cursor right), and 0x01 (0000 0001 — Clear Display).

-- end of AI slop --

0x06 -> Entry Mode Set :            0000'01 1 (forward) 0 (shift, the whole sh!t moves away when u type wtf)
0x0C -> Display On/Off Control :    0000'1 1 (Display) 0 (Cursor _) 0 (blinking when typing, ugly asf imo)
0x28 -> Function Set :              001 0 (stay in 4-bit mode) 1 (number of lines, use both) 0 (font size, idk) XX (unused)

---

snprintf format: "%-16.16s"

%[...]s: string

-: Align text on the left

16.: Minimum 16 characters, fills the rest with spaces/blank

.16: Max 16, yeets the rest away

---

btw uh don't ask me why i named the c file main.c and the header file lcd.h
it made sense to me at some point but i forgor

---

how uint32_t utf_to_unicode works
basically, if lcd_update() is called with a string containing a special characters
it smartly tries to check if it matches a custom characters to avoid sending
broken characters (happened to me when using french accents like é in Février)
lemme drop some useful info i found: https://stackoverflow.com/a/643810
disclamer: used ai for this function as i couldn't figure it out (skill issue)
also used ai to explain how it works, below:

`utf_to_unicode()` reads **one complete UTF-8 character** from a C string starting at index `*idx`, converts its 1 to 4 bytes into a single Unicode ID number (called a **code point**, like `0x2103` for `℃`), and advances `*idx` to the first byte of the next character.

In C, a normal `for (int i = 0; s[i] != '\0'; i++)` loop steps **one byte at a time**. That works for plain ASCII (where 1 character = 1 byte), but breaks on UTF-8 symbols like `"℃"` (3 bytes) or `"é"` (2 bytes)—treating a single visible symbol as multiple separate characters. `utf_to_unicode()` lets your loop step **one visible character at a time** instead.

---

### How UTF-8 Encodes Length in the First Byte

UTF-8 is designed so the leading bits of the first byte (`b0`) tell you exactly how many bytes belong to that character, while every trailing byte starts with the bits `10xxxxxx`:

| Byte Length | First Byte (`b0`) Pattern | Trailing Byte(s) Pattern | Payload Bits Extracted | `*idx` Advance |
| --- | --- | --- | --- | --- |
| **1 byte** (ASCII) | `0xxxxxxx` (`< 0x80`) | *(none)* | 7 bits (`b0`) | `+ 1` |
| **2 bytes** (e.g., `é`, `°`) | `110xxxxx` (`(b0 & 0xE0) == 0xC0`) | `10xxxxxx` | 5 + 6 = **11 bits** | `+ 2` |
| **3 bytes** (e.g., `℃`, `→`) | `1110xxxx` (`(b0 & 0xF0) == 0xE0`) | `10xxxxxx` `10xxxxxx` | 4 + 6 + 6 = **16 bits** | `+ 3` |
| **4 bytes** (e.g., emojis) | `11110xxx` (`(b0 & 0xF8) == 0xF0`) | `10xxxxxx` × 3 | 3 + 6 + 6 + 6 = **21 bits** | `+ 4` |

*(If the bytes don't match any valid pattern—for example, corrupted data—the fallback at the bottom skips 1 byte with `(*idx) += 1` and returns `'?'` so a `while` loop can never get stuck.)*

---

### Step-by-Step Walkthrough: Decoding `"℃"` (`U+2103`)

In memory, the string `"℃"` is stored as three bytes: `0xE2`, `0x84`, `0x83` (`11100010` `10000100` `10000011`).

1. **Detect the 3-byte header:**
* `b0` is `0xE2` (`1110 0010`).
* Masking off the bottom 4 bits with `b0 & 0xF0` gives `0xE0` (`1110 0000`), matching the 3-byte prefix `1110xxxx`.
* Checking `(s[*idx + 1] & 0xC0) == 0x80` and `(s[*idx + 2] & 0xC0) == 0x80` confirms both following bytes start with `10xxxxxx` (and aren't `'\0'`).


2. **Strip the UTF-8 wrapper bits to get the raw payload:**
* Byte 0 (`0xE2 & 0x0F`): drops `1110`, keeping **`0010`** (`0x02`)
* Byte 1 (`0x84 & 0x3F`): drops `10`, keeping **`000100`** (`0x04`)
* Byte 2 (`0x83 & 0x3F`): drops `10`, keeping **`000011`** (`0x03`)


3. **Stitch the payload bits back together with shifts (`<<`) and bitwise OR (`|`):**
```c
uint32_t unicode = ((uint32_t)(b0 & 0x0F) << 12) |       // 0x02 << 12 = 0x2000
              ((uint32_t)(s[*idx + 1] & 0x3F) << 6) | // 0x04 << 6  = 0x0100
              (uint8_t)(s[*idx + 2] & 0x3F);          //            = 0x0003
// Result: 0x2000 | 0x0100 | 0x0003 = 0x2103 (U+2103, '℃')

```


4. **Advance the caller's index:**
* `(*idx) += 3` moves the caller's position past all three bytes at once, and the function returns `0x2103` so `unicode_to_rom()` knows which single glyph to send to the LCD.