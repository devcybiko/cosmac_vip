#include "Game.h"
#include <Arduboy2.h>
#include "Assets.h"

extern Arduboy2 arduboy;          // defined in main.cpp

void gameInit() { /* init state */ }

void gameUpdate() {
  if (arduboy.pressed(LEFT_BUTTON))  { /* ... */ }
  if (arduboy.pressed(RIGHT_BUTTON)) { /* ... */ }
}

void gameDraw() {
  // draw something...
  arduboy.setCursor(0, 0);
  arduboy.print(F("Hello from .cpp!"));
  // Sprites::drawOverwrite(x, y, PLAYER_BMP, 0);
}
