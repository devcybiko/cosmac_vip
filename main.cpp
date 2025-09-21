#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "cdp1802.h"
#include "rom.h"
#include "spaceship.h"
#include "oled32.h"

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

uint32_t lastBlink = 0;
bool ledOn = false;
static cdp1802* cdp;
static uint8_t* fb;
static uint8_t fb2[256];

unsigned char mget(unsigned short addr) {
  return spaceship[addr & 0xff];
  // return rom[addr & 0xff];
}
static uint16_t y;
static uint16_t x;
static uint16_t y_offset = 0;

void mset(unsigned short addr, unsigned char byte) {
  if (addr > 256) {
    addr = addr % 256;
    unsigned short m = 0;
    unsigned short r = addr % 8;
    unsigned short c = addr / 8;
    c *= 2;
    m = (7 - r) * 128 + c + y_offset;
    fb[m++] = byte;
    fb[m] = byte;
  } else {
    // rom[addr] = byte;
  }
}

void handle_buttons() {
  uint8_t buttons = arduboy.buttonsState();
  cdp->flags[_EF1] = buttons & UP_BUTTON;
  cdp->flags[_EF2] = buttons & DOWN_BUTTON;
  cdp->flags[_EF3] = buttons & LEFT_BUTTON;
  cdp->flags[_EF4] = buttons & RIGHT_BUTTON;
  cdp->flags[_NEF1] = !cdp->flags[_EF1];
  cdp->flags[_NEF2] = !cdp->flags[_EF2];
  cdp->flags[_NEF3] = !cdp->flags[_EF3];
  cdp->flags[_NEF4] = !cdp->flags[_EF4];
}
void handle_q() {
  if (cdp->flags[_QF]) {
    arduboy.setRGBled(32, 0, 0);  // faint red blink every 0.5s
    sound.tone(600, 0);
  } else {
    arduboy.setRGBled(0, 0, 0);  // faint red blink every 0.5s
    sound.noTone();
  }
}
void setup() {
  // Must initialize the hardware & display
  arduboy.begin();  // shows boot logo for ~2s if not disabled in system settings
  // arduboy.safeMode();
  arduboy.audio.begin();      // sets up audio + reads saved on/off
  arduboy.audio.on();         // force-enable
  arduboy.audio.saveOnOff();  // persist ON in EEPROM

  // arduboy.flipHorizontal(true);
  // arduboy.flipVertical(true);
  arduboy.setFrameRate(8);  // keep it simple
  arduboy.clear();
  arduboy.setCursor(0, 54);
  arduboy.fillRect(64, 0, 64, 64, 1);

  arduboy.display();  // push first frame immediately
  fb = arduboy.getBuffer();
  cdp = cdp1802_init(mget, mset);
}


void blink() {
  uint32_t now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    ledOn = !ledOn;
    arduboy.setRGBled(ledOn ? 32 : 0, 0, 0);  // faint red blink every 0.5s
    // if (ledOn) {
    // sound.tone(500, 525);
    // }
    // else {
    //   sound.noTone();
    // }
  }
}

void _debug_() {
    arduboy.clear();  // Clear the entire screen first
    cdp = cdp1802_info();
    for(int i=0, y=0; i<8; i++, y+=8) {
      arduboy.setCursor(0, y);
      arduboy.print("R"); arduboy.print(i); arduboy.print(":"); arduboy.println(cdp->R[i], HEX);
    }
    for(int i=0, y=0; i<8; i++, y+=8) {
      arduboy.setCursor(48, y);
      arduboy.print("F"); arduboy.print(i); arduboy.print(":"); arduboy.println(cdp->flags[i], HEX);
    }
    for(int i=0, y=0; i<8; i++, y+=8) {
      arduboy.setCursor(80, y);
      arduboy.print("d"); arduboy.print(i); arduboy.print(":"); arduboy.println(cdp->debug[i], HEX);
    }
    arduboy.display();  // Display everything at the end
}
void loop() {
  // Simple animated heartbeat so you KNOW it's running:
  blink();
  if (arduboy.nextFrame()) {
    displayColumns(arduboy, 0, 64);
    uint8_t buttons = arduboy.buttonsState();
    if (buttons & A_BUTTON) y_offset++;
    if (buttons & B_BUTTON) y_offset--;
  }
  handle_buttons();
  cdp1802_dispatch();
  handle_q();
}
