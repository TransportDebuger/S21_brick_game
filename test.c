#include <stdio.h>

#include "./brick_game/tetris/tetris.h"

int main() {
  Game_t* tetg = createGame();
  setCellValue(tetg->gameInfo->field, 1, 1, 1);
  for (int i = 0; i < GAME_BOARD_HEIGHT; i++) {
    for (int j = 0; j < GAME_BOARD_WIDTH; j++) {
      printf("%i ", getCellValue(tetg->gameInfo->field, i, j));
    }
    printf("\n");
  }
  destroyGame(tetg);
  return 0;
}