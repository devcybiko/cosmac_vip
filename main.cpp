#include <Arduboy2.h>
#include "cdp1802.h"
#include "rom.h"

Arduboy2 arduboy;

uint32_t lastBlink = 0;
bool ledOn = false;
static cdp1802 *cdp;

unsigned char mget(unsigned short addr) {
  return rom[addr];
}

void mset(unsigned short addr, unsigned char byte) {
  rom[addr] = byte;
}

void setup() {
  // Must initialize the hardware & display
  arduboy.begin();              // shows boot logo for ~2s if not disabled in system settings
  arduboy.setFrameRate(30);     // keep it simple
  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print(F("Hello COSMAC VIP!"));
  arduboy.display();            // push first frame immediately
  cdp = cdp1802_init(mget, mset);
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();

  // Simple animated heartbeat so you KNOW it's running:
  uint32_t now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    ledOn = !ledOn;
    arduboy.setRGBled(ledOn ? 32 : 0, 0, 0);  // faint red blink every 0.5s
  }

  // Draw something every frame
  static int x = 0;
  int y = 0;
  x = (x + 2) % 128;

  arduboy.clear();
  arduboy.setCursor(0, y);
  arduboy.print(F("Hello COSMAC VIP!"));
  y += 20;
  arduboy.drawFastHLine(0, y, 128);   // a line
  y += 4;
  arduboy.fillRect(x, y, 10, 10);     // moving block
  y += 12;
  arduboy.setCursor(0, y);
  arduboy.print("R0:");
  arduboy.print(cdp->R[0], HEX);
  arduboy.print(" D:");
  arduboy.print(cdp->D, HEX);
  arduboy.print(" M:");
  arduboy.print(cdp->mget(0), HEX);
  arduboy.display();
  cdp1802_dispatch();
}
