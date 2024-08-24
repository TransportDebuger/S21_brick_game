#include "./tetris.h"

GameInfo_t *createGameInfo() {
  GameInfo_t *game = (GameInfo_t *)malloc(sizeof(GameInfo_t));
  game->field = createGameField(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH);
  game->next = createGameField(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH);
  game->score = 0;
  game->level = 1;
  game->speed = 0;
  game->pause = 0;
  // game->high_score = loadHightScore();
  return game;
}

int **createGameField(const int rows, const int cols) {
  int **field = (int **)calloc((size_t)rows, sizeof(int *));
  for (int i = 0; i < rows; i++) {
    *(field + i) = (int *)calloc((size_t)cols, sizeof(int));
  }
  return field;
}

void destroyGameInfo(GameInfo_t *game) {
  if (game) {
    destroyGameField(game->field, GAME_BOARD_HEIGHT);
    free(game);
  }
}

void destroyGameField(int **field, const int rows) {
  if (field != NULL) {
    for (int i = 0; i < rows; i++) {
      free(field[i]);
    }
    free(field);
  }
}

//void updateGameState(GameInfo_t *game){};
//void updateGame(GameInfo_t *game) {}

/* Забираем значение ячейки игрового поля */
int getCellValue(int **gameboard, int colindex, int rowindex) {
  int val = -1;
  if (gameboard && colindex >= 0 && colindex < GAME_BOARD_WIDTH &&
      rowindex >= 0 && rowindex < GAME_BOARD_HEIGHT) {
    val = gameboard[colindex][rowindex];
  }
  return val;
}

/* Устанавливаем значение игровой ячейки */
int setCellValue(int **gameboard, int colindex, int rowindex, int val) {
  int err = 0;
  if (gameboard && colindex >= 0 && colindex < GAME_BOARD_WIDTH &&
      rowindex >= 0 && rowindex < GAME_BOARD_HEIGHT) {
    gameboard[colindex][rowindex] = val;
  } else {
    err = 1;
  }
  return err;
}

// int loadHighScore() { return 0; }

// void saveHighScore(const int hscore) {}