#include <Arduboy2.h>
#include "cdp1802.h"
#include "rom.h"
#include "spaceship.h"

Arduboy2 arduboy;

uint32_t lastBlink = 0;
bool ledOn = false;
static cdp1802 *cdp;
static uint8_t* fb;
static uint8_t fb2[256];

unsigned char mget(unsigned short addr) {
  return spaceship[addr & 0xff];
  // return rom[addr & 0xff];
}
static    uint16_t y;
static    uint16_t x;

void mset(unsigned short addr, unsigned char byte) {
  if (addr > 256) {
    addr = addr % 256;
    unsigned short m = 0;
    unsigned short r = addr % 8;
    unsigned short c = addr / 8;
    c *= 2;
    m = (7-r) * 128 + c;
    fb[m] = byte;
    m = (7-r) * 128 + c + 1;
    fb[m] = byte;
  } else {
    // rom[addr] = byte;
  }
}

void setup() {
  // Must initialize the hardware & display
  arduboy.begin();              // shows boot logo for ~2s if not disabled in system settings
  arduboy.setFrameRate(30);     // keep it simple
  arduboy.clear();
  arduboy.setCursor(0, 54);
  // arduboy.print(F("COSMAC VIP EMU!"));
  arduboy.display();            // push first frame immediately
  fb = arduboy.getBuffer();
  cdp = cdp1802_init(mget, mset);
}



void loop() {
  // Simple animated heartbeat so you KNOW it's running:
  uint32_t now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    ledOn = !ledOn;
    arduboy.setRGBled(ledOn ? 32 : 0, 0, 0);  // faint red blink every 0.5s
  }

  if (arduboy.nextFrame())  {
      arduboy.display();
  }
  cdp1802_dispatch();

}
