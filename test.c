#include <stdio.h>
#include <stdlib.h>

#include "./brick_game/tetris/tetris.h"

int main() {
  GameInfo_t* tetg = createGameInfo();
  setCellValue(tetg->field, 1, 1, 1);
  for (int i = 0; i < GAME_BOARD_HEIGHT; i++) {
    for (int j = 0; j < GAME_BOARD_WIDTH; j++) {
      printf("%i ", getCellValue(tetg->field, i, j));
    }
    printf("\n");
  }
  destroyGameInfo(tetg);
  return 0;
}