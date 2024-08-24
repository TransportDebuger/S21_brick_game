#ifndef TETRIS_H
#define TETRIS_H

#include <stdlib.h>

#include "gamepref.h"

// Game preferences

typedef enum UserAction_t {
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Action
} UserAction_t;

typedef enum Tetraminoes_i {
  itype,
  otype,
  ttype,
  ltype,
  jtype,
  stype,
  ztype
} Tetraminoes_i;

typedef struct Tetramino {
  int *data;
  int side;
} Tetramino;

int i_tetramino[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

int o_tetramino[] = {2, 2, 2, 2};

int t_tetramino[] = {0, 0, 0, 3, 3, 3, 0, 3, 0};

int l_tetramino[] = {0, 0, 0, 4, 4, 4, 4, 0, 0};

int j_tetramino[] = {0, 0, 0, 5, 5, 5, 0, 0, 5};

int s_tetramino[] = {0, 0, 0, 0, 6, 6, 6, 6, 0};

int z_tetramino[] = {0, 0, 0, 7, 7, 0, 0, 7, 7};

const Tetramino Tetraminoes[] = {
    {i_tetramino, 4}, {o_tetramino, 2}, {t_tetramino, 3}, {l_tetramino, 3},
    {j_tetramino, 3}, {s_tetramino, 3}, {z_tetramino, 3}};

typedef struct TetraminoState_t {
  int tetraminoIndex;
  int offsetRow;
  int offesetCol;
  int orientation;
} TetraminoState_t;

typedef struct GameInfo_t {
  int **field;  // Game field
  int **next;   // 
  int score;    // ScorePoints
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo_t;

// Game constructors
GameInfo_t *createGameInfo();
int **createGameField(const int rows, const int cols);

// Game mechanics
// void updateCurrentState(GameInfo_t *curGame);
// void updateGame(GameInfo_t *curGame);
int getCellValue(int **gameboard, int col, int row);
int setCellValue(int **gameboard, int col, int row, int val);
//void userInput(UserAction_t action, bool hold);

// Game destructors
void destroyGameInfo(GameInfo_t *game);
void destroyGameField(int **field, const int rows);

//Функции загрузки и выгруки лучшего результата
// int loadHighScore();
// void saveHighScore(const int hscore);

// GameInfo_t updateCurrentState();

#endif