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
  return gameboard[col][row];
}

/* Устанавливаем значение игровой ячейки */
void setCellValue(int **gameboard, int col, int row, int val) {
  gameboard[col][row] = val;
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