// ====== Arduboy bare-metal framebuffer driving (SSD1306 over SPI) ======
#include <Arduino.h>
#include <avr/io.h>

#define PIN_DC   4   // Data/Command
#define PIN_RST  6   // Reset
#define PIN_CS   12  // Chip Select

// 128x64 mono framebuffer: 1 bit per pixel, arranged in pages (8 rows per byte).
static uint8_t FB[32 * 64 / 8];

// ---- Low-level SPI (uses hardware SPI on ATmega32u4) ----
static inline void spiBegin() {
  // Set MOSI + SCK + SS (CS pin) as outputs.
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);

  // The ATmega32u4 SPI pins are fixed (SCK/MOSI on port B).
  // Enable SPI, Master mode, Clock = F_CPU/2 (set double speed)
  SPCR = _BV(SPE) | _BV(MSTR);   // enable SPI, master
  SPSR = _BV(SPI2X);             // double speed (Fosc/2)
}

static inline void spiTx(uint8_t b) {
  SPDR = b;
  while (!(SPSR & _BV(SPIF))) { /* wait */ }
}

// ---- OLED helpers ----
static inline void oledCommand(uint8_t c) {
  digitalWrite(PIN_DC, LOW);
  digitalWrite(PIN_CS, LOW);
  spiTx(c);
  digitalWrite(PIN_CS, HIGH);
}

static inline void oledData(uint8_t d) {
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_CS, LOW);
  spiTx(d);
  digitalWrite(PIN_CS, HIGH);
}

static void oledInit() {
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_CS, OUTPUT);

  spiBegin();

  // Hardware reset
  digitalWrite(PIN_RST, LOW);  delay(10);
  digitalWrite(PIN_RST, HIGH); delay(10);

  // SSD1306 init sequence (SPI, 128x64, horizontal addressing)
  oledCommand(0xAE);            // DISPLAYOFF
  oledCommand(0xD5); oledCommand(0x80); // SETDISPLAYCLOCKDIV (suggested ratio)
  oledCommand(0xA8); oledCommand(0x3F); // SETMULTIPLEX (0x3F for 64)
  oledCommand(0xD3); oledCommand(0x00); // SETDISPLAYOFFSET = 0
  oledCommand(0x40 | 0x00);     // SETSTARTLINE = 0
  oledCommand(0x8D); oledCommand(0x14); // CHARGEPUMP on (internal)
  oledCommand(0x20); oledCommand(0x00); // MEMORYMODE = horizontal
  oledCommand(0xA1);            // SEGREMAP (mirror horizontally) — flip if needed
  oledCommand(0xC8);            // COMSCANDEC (flip vertically)   — flip if needed
  oledCommand(0xDA); oledCommand(0x12); // SETCOMPINS (alt COM pins, 128x64)
  oledCommand(0x81); oledCommand(0xCF); // SETCONTRAST
  oledCommand(0xD9); oledCommand(0xF1); // SETPRECHARGE
  oledCommand(0xDB); oledCommand(0x40); // SETVCOMDETECT
  oledCommand(0xA4);            // DISPLAYALLON_RESUME
  oledCommand(0xA6);            // NORMALDISPLAY (A7 for inverted)
  oledCommand(0x2E);            // DEACTIVATE SCROLL
  oledCommand(0xAF);            // DISPLAYON
}

// Set the column/page window to full screen
static inline void oledSetFullWindow() {
  oledCommand(0x21); oledCommand(0x00); oledCommand(0x7F); // COLUMNADDR 0..127
  oledCommand(0x22); oledCommand(0x00); oledCommand(0x07); // PAGEADDR   0..7
}

// Push the whole framebuffer to the OLED
static void flush() {
  oledSetFullWindow();
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_CS, LOW);
  // Send all 1024 bytes
  for (uint16_t i = 0; i < sizeof(FB); ++i) {
    spiTx(FB[i]);
  }
  digitalWrite(PIN_CS, HIGH);
}

// Clear framebuffer (RAM only)
static inline void fbClear() {
  memset(FB, 0x00, sizeof(FB));
}
static inline uint8_t *fbRAM() {
  return FB;
}
// Set/clear a pixel in the framebuffer
static inline void setPixel(uint8_t x, uint8_t y, bool on) {
  if (x >= 128 || y >= 64) return;
  uint16_t index = (y >> 3) * 128 + x; // page*width + x
  uint8_t  mask  = (1 << (y & 7));
  if (on)  FB[index] |=  mask;
  else     FB[index] &= ~mask;
}

// // Simple demo: draw a moving 10x10 block
// void setup() {
//   // Power switch must be ON on the Arduboy!
//   oledInit();
//   fbClear();

//   // Draw some text pixels "HELLO" very crudely as a sanity mark
//   for (uint8_t i = 0; i < 30; ++i) setPixel(2 + i, 2, true);
//   flush();
// }

// void loop() {
//   static uint8_t x = 0;
//   static uint32_t t0 = 0;

//   if (millis() - t0 < 33) return; // ~30 FPS
//   t0 = millis();

//   fbClear();
//   // moving box
//   for (uint8_t yy = 30; yy < 40; ++yy)
//     for (uint8_t xx = x; xx < x + 10; ++xx)
//       setPixel(xx, yy, true);

//   flush();
//   x = (uint8_t)((x + 2) % (128 - 10));
// }
