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

## Keyboard Emulation

I haven't looked at the Arduboy I/O library yet to emulate a keyboard. I know that 1802 projects cover a wide range of hardware configurations, so emulating every possibility at that level is likely out of scope. 


