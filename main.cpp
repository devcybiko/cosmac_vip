#include <Arduboy2.h>
#include "cdp1802.h"
#include "rom.h"

Arduboy2 arduboy;

uint32_t lastBlink = 0;
bool ledOn = false;
static cdp1802 *cdp;
static uint8_t* fb;

unsigned char mget(unsigned short addr) {
  return rom[addr & 0xff];
}

void mset(unsigned short addr, unsigned char byte) {
  if (addr > 256) {
    fb[addr % 256] = byte;
  } else {
    rom[addr] = byte;
  }
}

void setup() {
  // Must initialize the hardware & display
  arduboy.begin();              // shows boot logo for ~2s if not disabled in system settings
  arduboy.setFrameRate(30);     // keep it simple
  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print(F("Hello COSMAC VIP!"));
  arduboy.display();            // push first frame immediately
  fb = arduboy.getBuffer();
  cdp = cdp1802_init(mget, mset);
}



void loop() {
  if (arduboy.nextFrame()) 
    arduboy.display();
  // Simple animated heartbeat so you KNOW it's running:
  uint32_t now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    ledOn = !ledOn;
    arduboy.setRGBled(ledOn ? 32 : 0, 0, 0);  // faint red blink every 0.5s
  }
  cdp1802_dispatch();
}
