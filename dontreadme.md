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