# cosmac_vip on arduboy

This was put together on a lark over a weekend. So I'm sure improvements can be made.

## Switch statements

Strangely I had previously used a switch() statement to decode the instructions, but it failed to work with 16 case statements. I'm not sure why. I couldn't find any documentation on broken switch statements on Arduino. So I had CoPilot convert them to if-hydras.

## Memory Limitations

I'm looking forward to eeking out better performance. I am also concerned about the lack of memory. There's only about 2.5K onboard and 1K is taken up by the display. (So we may have to read and write instructions in the display memory. Just like the original VIP! How meta.)

The mget()/mset() is foreshadowing reading 1802 instructions from the Flash - acting as a ROM. I thought I might use a page-swapping strategy like Virtual Memory systems. So, there may be 4 pages of 256-byte "RAM" and when you request a page that is not in-memory, it will read from the Flash and execute that. The initial load would be slow, but once in-memory it should be performant. Especially since most 1802 code is designed to run in 256-byte segments anyway. This approach would make full use of the 32KB Flash for larger programs (or 16MB for Arduboy FX).

In fact, if you look at mset() in its current implementation, it actually detects when you're writing outside the low-256 byte segment and redirects writing to the display. I plan a similar approach for mget().

## Display Emulation

I had hoped to write directly to the OLED (see oled.cpp), but you cannot read the OLED display and so you must have a backing frame buffer anyway. I might be able to write my own frame buffer handler (using just 64x32 pixels) and thus returning 768 bytes of RAM to the emulator. And since we're only transferring 256 bytes (instead of 1K) to the OLED then we also get a speed bump.

## Keyboard Emulation

I haven't looked at the Arduboy I/O library yet to emulate a keyboard. I know that 1802 projects cover a wide range of hardware configurations, so emulating every possibility at that level is likely out of scope. 


