#ifndef TETRIS_H
#define TETRIS_H

#include <stdlib.h>

#include "../../gamepref.h"

// Game preferences

typedef enum UserAction_t {
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Rotate
} UserAction_t;

typedef enum Tetraminoes {
  Itype,
  Otype,
  Ttype,
  Ltype,
  Jtype,
  Stype,
  Ztype
} Tetraminoes;

typedef struct GameInfo_t {
  int **field; // Game field
  int **next;
  int score; // ScorePoints
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo_t;

// Game constructors
GameInfo_t *CreateGameInfo(const int frows, const int fcols);
int **CreateGameField(const int rows, const int cols);

// Game mechanics
void updateGameState(GameInfo_t *curGame);
void updateGame(GameInfo_t *curGame);
int getCellValue(GameInfo_t *game, int col, int row);
void setCellValue(int **gameboard, int col, int row, int val);

// Game destructors
void DestroyGameInfo(GameInfo_t *game);
void DestroyGameField(int **field, const int rows);

// void userInput(UserAction_t action, bool hold);

//Функции загрузки и выгруки лучшего результата
int loadHighScore(GameInfo_t *ginfo);
void saveHighScore(const GameInfo_t *ginfo);

// GameInfo_t updateCurrentState();

#endif