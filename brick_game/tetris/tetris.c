#include "./tetris.h"

GameInfo_t *createGameInfo() {
  GameInfo_t *game = (GameInfo_t *)malloc(sizeof(GameInfo_t));
  game->field = createGameField(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH);
  game->next = NULL;
  game->score = 0;
  game->level = 1;
  game->speed = 0;
  game->pause = 0;
  // game->high_score = loadHightScore();
  return game;
}

void destroyGameInfo(GameInfo_t *game) {
  if (game) {
    destroyGameField(game->field);
    free(game);
  }
}

int **createGameField(const int rows, const int cols) {
  int **field = (int **)calloc((size_t)rows, sizeof(int *));
  int *cells = (int *)calloc((size_t)rows * (size_t)cols, sizeof(int));
  for (int i = 0; i < rows; i++) {
    field[i] = cells + i * cols;
  }
  return field;
}

void destroyGameField(int **field) {
  if (field != NULL) {
    free(*field);
    free(field);
  }
}

// void updateGameState(GameInfo_t *game){};
// void updateGame(GameInfo_t *game) {}

/*
  Забираем значение ячейки игрового поля
*/
int getCellValue(int **gameboard, int rowindex, int colindex) {
  int val = -1;
  if (gameboard && colindex >= 0 && colindex < GAME_BOARD_WIDTH &&
      rowindex >= 0 && rowindex < GAME_BOARD_HEIGHT) {
    val = gameboard[rowindex][colindex];
  }
  return val;
}

/*
  Устанавливаем значение игровой ячейки. Значение val берется из массива,
  отрисовывающего фигуру.
*/
int setCellValue(int **gameboard, int rowindex, int colindex, int val) {
  int err = 0;
  if (gameboard && colindex >= 0 && colindex < GAME_BOARD_WIDTH &&
      rowindex >= 0 && rowindex < GAME_BOARD_HEIGHT) {
    gameboard[rowindex][colindex] = val;
  } else {
    err = 1;
  }
  return err;
}

/*
  Изменение состояния при вращении.
  Изменяется аттрибут orientation структуры TetraminoState_t, исходя из
  направления вращения. Позиции пронумерованы от 0 до 4 начиная с верхнего
  направления по часовой стрелке.
*/
int rotateTetramino(TetraminoState_t *tetSate, const int direction) {
  int err = 0;
  if (tetSate) {
    if (tetSate->orientation == 3)
      tetSate->orientation = 0;
    else
      tetSate->orientation++;
    if (tetSate->orientation == 0)
      tetSate->orientation = 3;
    else
      tetSate->orientation--;
  } else
    err = 1;
  //Добавить обработку коллизии при повороте фигуры и соответственно ее смещение
  //относительно вертикали
  return err;
}

int moveTetramino(GameInfo_t *gameinfo, TetraminoState_t *tetState,
                  const int direction) {
  int state = -1;
  if (gameinfo && tetState) {
    if (gameinfo->field) {
      if (direction == 1) {
        //Движение вниз
        tetState->offsetRow++;
        if (checkCollision(gameinfo, tetState)) {
          tetState->offsetRow--;
          state = 2;
        }
      } else if (direction == 2) {
        tetState->offsetCol--;
        //проверка достижения левой границы
        if (checkCollision(gameinfo, tetState)) {
          tetState->offsetCol++;
          state = 0;
        }
      } else if (direction == 3) {
        tetState->offsetCol++;
        //проверка достижения правой границы
        if (checkCollision(gameinfo, tetState)) {
          tetState->offsetCol--;
          state = 0;
        }
      }
    }
  }

  return state;
}

int checkCollision(GameInfo_t *gameinfo, TetraminoState_t *tetState) {
  int isCollided = 0;

  return isCollided;
}
// int loadHighScore() { return 0; }
// void saveHighScore(const int hscore) {}