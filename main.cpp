#include <Arduboy2.h>
#include "cdp1802.h"
#include "rom.h"

Arduboy2 arduboy;

uint32_t lastBlink = 0;
bool ledOn = false;

void setup() {
  // Must initialize the hardware & display
  arduboy.begin();              // shows boot logo for ~2s if not disabled in system settings
  arduboy.setFrameRate(30);     // keep it simple
  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print(F("Hello COSMAC VIP!"));
  arduboy.display();            // push first frame immediately
  cdp1802_init();
  cdp1802 *info = cdp1802_info();
  memcpy(info->M, rom, sizeof(rom));
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
  x = (x + 2) % 128;

  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print(F("Hello COSMAC VIP!"));
  arduboy.drawFastHLine(0, 20, 128);   // a line
  arduboy.fillRect(x, 32, 10, 10);     // moving block
  cdp1802 *info = cdp1802_info();
  arduboy.setCursor(0, 50);
  arduboy.print("R0: 0x");
  arduboy.println(info->R[0], HEX);
  arduboy.setCursor(64, 50);
  arduboy.print("D: 0x");
  arduboy.println(info->D, HEX);
  arduboy.display();
  cdp1802_dispatch();
}
