/*!
  \file
  \brief Основной файл проекта tetris

  \author provemet
  \version 1
  \date September 2024
  Данный файл содержит в себе...
*/

#include "main.h"

#include "./brick_game/tetris/tetris.h"
#include "./gui/cli/graphic.h"
#include "./gui/cli/splash.h"

int main() {
  // Enable ncurses
  initGraphics();
  GameInfo_t* game = createGameInfo();
  WINDOW* gamewin = newwin(SPLASH_HEIGHT, SPLASH_WIDTH, 0, 0);
  drawSplash(gamewin);
  if (gamewin) {
    delwin(gamewin);
  }

  // How to call Main Game Function
  destroyGameInfo(game);
  deinitGraphics();
  return 0;
};