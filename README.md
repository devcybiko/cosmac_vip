# cosmac_vip on arduboy

This was put together on a lark over a weekend. So I'm sure improvements can be made.

## Design Strategy

The CDP-1802 is often called a "symmetric" processor architecture because it has a small set of instructions that work on the 16 (0-F) registers equally (well mostly).

The 8-bit op-codes are broken down into two "nybbles": "I" (or instruction) is in the upper nybble. And "N" for the register the instruction is operating on. Notable deviations are the Branch (3x, Cx), Logical (7x, Fx), and I/O (6x) instructions.

So, the main emulation code reads a byte from memory, splits it into "I" and "N", then a switch/case statement on "I" allows executing on each "N" register (which are stored in an array of shorts).

## Switch statements

Strangely I had previously used a switch() statement to decode the instructions, but it failed to work with 16 case statements. I'm not sure why. I couldn't find any documentation on broken switch statements on Arduino. So I had CoPilot convert them to if-hydras.

## Memory Limitations

I'm looking forward to eeking out better performance. I am also concerned about the lack of memory. There's only about 2.5K on-board and 1K is taken up by the display. (So we may have to read and write instructions in the display memory. Just like the original ELF! How meta.)

To allow alternative memory configurations, I read/write memory using mget() and mset() functions. The downside is function call overhead on each memory access.

The mget()/mset() functions are foreshadowing reading 1802 instructions from the Flash - acting as a ROM. I thought I might use a page-swapping strategy like Virtual Memory systems. So, there may be 4 pages of 256-byte "RAM" and when you request a page that is not in-memory, it will read from the Flash and execute that. The initial load would be slow, but once in-memory it should be performant. Especially since most 1802 code is designed to run in 256-byte segments anyway. This approach would make full use of the 32KB Flash for larger programs (or 16MB for Arduboy FX).

In fact, if you look at mset() in its current implementation, it actually detects when you're writing outside the low-256 byte segment and redirects writing to the display. I plan a similar approach for mget().

## Display Emulation

I had hoped to write directly to the OLED (see oled.cpp), but you cannot read the OLED display and so you must have a backing frame buffer anyway. I might be able to write my own frame buffer handler (using just 64x32 pixels) and thus returning 768 bytes of RAM to the emulator. And since we're only transferring 256 bytes (instead of 1K) to the OLED then we also get a speed bump.

The current version of the display `mset()` function doubles the vertical pixels of the screen for readability.

I have written a bit of code that writes only half the frame buffer to the OLED. I was hoping for a performance boost, but it's clear that the CDP1802 emulator is the big performance hog, not the OLED transfer.

However, as mentioned earlier, we can scarf up some of the unused video memory for emulated memory. That will come later.

## Keyboard Emulation

Because the graphics are rotated 90 degrees to accommodate the OLED's memory configuration, I've mapped the D-Pad as follows:

| Rotated | EFlag | d-pad |
|-------|-------|--------|
| UP |  EF1 | left |
| DOWN | EF2 | right |
| LEFT | EF3 | down |
| RIGHT | EF4 | up |

I have plans to map the entire button bitmap (including A_BUTTON and B_BUTTON from arduboy.buttonsState()) to one of the INPUT opcodes.

For the demo (spaceship.h) I have hardwired the UP_BUTTON and DOWN_BUTTON to a `y_offset` variable to experiment with offset for the display.

## Q-Flag and Hardware Emulation

The majority of hardware emulation deals with the keyboard buttons and the display. I mapped the `Q` flag to the speaker and LED. So you get a "realistic" squeal from the EMU when `Q` is set.
    
## TODO and Future Plans

1. Virtual Memory
2. FLASH Memory reading (ROM)
3. CHIP-8 
4. Multiple programs stored in FLASH
5. User uploads of programs (.hex files?)
6. Performance