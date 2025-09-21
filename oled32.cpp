#include "oled32.h"

// SSD1306 helpers via Arduboy2Core
static inline void oledCmd(uint8_t c) {
  Arduboy2Core::LCDCommandMode();
  Arduboy2Core::SPItransfer(c);
}
static inline void oledDataBegin() { Arduboy2Core::LCDDataMode(); }
static inline void oledData(uint8_t d) { Arduboy2Core::SPItransfer(d); }

// Set column (x) range and one page (y/8)
static inline void setColPage(uint8_t col0, uint8_t col1, uint8_t page) {
  oledCmd(0x21); oledCmd(col0); oledCmd(col1);   // COLUMNADDR col0..col1
  oledCmd(0x22); oledCmd(page); oledCmd(page);   // PAGEADDR   page..page
}

void displayColumns(Arduboy2& ab, uint8_t x0, uint8_t width) {
  const uint8_t *buf = ab.getBuffer();
  uint8_t x1 = x0 + width - 1;
  for (uint8_t page = 0; page < 8; ++page) {
    setColPage(x0, x1, page);
    oledDataBegin();
    const uint8_t *src = buf + page * 128 + x0;
    for (uint8_t x = 0; x < width; ++x) oledData(src[x]);
  }
}
