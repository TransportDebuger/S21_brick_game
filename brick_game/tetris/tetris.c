#include "./tetris.h"

GameInfo_t *CreateGameInfo(const int frows, const int fcols) {
  GameInfo_t *game = (GameInfo_t *)malloc(sizeof(GameInfo_t));
  game->field = CreateGameField(frows, fcols);
  game->score = 0;
  game->high_score = loadHightScore();
  return game;
}

void DestroyGameInfo(GameInfo_t *game) {
  if (game) {
    DestroyGameField(game->field, GAME_BOARD_HEIGHT);
    free(game);
  }
}

void updateGameState(GameInfo_t *game){};
void updateGame(GameInfo_t *game) {}

/* Забираем значение ячейки игрового поля */
int getCellValue(int **gameboard, int col, int row) {
  int val = -1;
  if (gameboard && col >= 0 && col < GAME_BOARD_WIDTH && row >= 0 &&
      row < GAME_BOARD_HEIGHT) {
    val = gameboard[col][row];
  }
  return val;
}

/* Устанавливаем значение игровой ячейки */
int setCellValue(int **gameboard, int col, int row, int val) {
  int err = 0;
  if (gameboard && col >= 0 && col < GAME_BOARD_WIDTH && row >= 0 &&
      row < GAME_BOARD_HEIGHT) {
    gameboard[col][row] = val;
  } else {
    err = 1;
  }
  return err;
}

int **CreateGameField(const int rows, const int cols) {
  int **field = (int **)calloc((size_t)rows, sizeof(int *));
  for (int i = 0; i < rows; i++) {
    *(field + i) = (int *)calloc((size_t)cols, sizeof(int));
  }
}

void DestroyGameField(int **field, const int rows) {
  if (field != NULL) {
    for (int i = 0; i < rows; i++) {
      free(field[i]);
    }
    free(field);
  }
}

int loadHighScore() { return 0; }

void saveHighScore(const int hscore) {}