#include "main.h"

#include "./brick_game/tetris/tetris.h"
#include "./gui/cli/graphic.h"
#include "./gui/cli/splash.h"

int main() {
  // Enable ncurses
  initGraphics();
  WINDOW *splash = newwin(SPLASH_HEIGHT, SPLASH_WIDTH, 0, 0);
  drawSplash(splash);
  if (splash) {
    delwin(splash);
  }
  // How to call Main Game Function
  deinitGraphics();
  return 0;
};