#ifndef TETRIS_H
#define TETRIS_H

#include <stdlib.h>

#include "gamepref.h"

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

typedef enum {
  I_TYPE,
  O_TYPE,
  T_TYPE,
  L_TYPE,
  J_TYPE,
  S_TYPE,
  Z_TYPE
} tertramino_index;

/*
  Структура описывающая фигуру. Так как каждая фигура выполняется в виде
  квадрата с определенной строной, то константа side задает его размерность по
  одной стороне. Константа data, содержит ссылку на линейный массив, в котором
  пустые ячейки обозначаются 0, заполненные любым отличным от 0 значением.
*/
typedef struct Tetramino {
  const int* data;
  const int side;
} Tetramino;

/*
  Блок констант описывающих массивы каждой фигуры.
*/
static const int i_type_tetramino[] = {0, 0, 0, 0, 1, 1, 1, 1,
                                       0, 0, 0, 0, 0, 0, 0, 0};

static const int o_type_tetramino[] = {2, 2, 2, 2};

static const int t_type_tetramino[] = {0, 0, 0, 3, 3, 3, 0, 3, 0};

static const int l_type_tetramino[] = {0, 0, 0, 4, 4, 4, 4, 0, 0};

static const int j_type_tetramino[] = {0, 0, 0, 5, 5, 5, 0, 0, 5};

static const int s_type_tetramino[] = {0, 0, 0, 0, 6, 6, 6, 6, 0};

static const int z_type_tetramino[] = {0, 0, 0, 7, 7, 0, 0, 7, 7};

/*
  Массив структур Tetraminо. Каждая фигура располагается по индексу
  соответсвующего значениям констант в перечислении tetramino_index
*/
static const Tetramino Tetraminoes[] = {
    {i_type_tetramino, 4}, {o_type_tetramino, 2}, {t_type_tetramino, 3},
    {l_type_tetramino, 3}, {j_type_tetramino, 3}, {s_type_tetramino, 3},
    {z_type_tetramino, 3}};

/*
  Структура, описывающая состояние отдельной фигуры на поле.
  Index - определяет какая фигура используется,
  offsetRow, offsetCol - определяет смещение относительно левого верхнего угла
  матрицы поля. orientation - определяет направление разворота.
*/
typedef struct TetraminoState_t {
  int tetraminoIndex;
  int offsetRow;
  int offsetCol;
  int orientation;
} TetraminoState_t;

/*
  Базовая структура состяния игры. Согласно условий ТЗ.
*/
typedef struct GameInfo_t {
  int** field;     // Game field
  int** next;      //
  int score;       // ScorePoints
  int high_score;  // Highscore points
  int level;       // current level
  int speed;       // delay between current end next cycle of game processing
  int pause;       // gamestate
} GameInfo_t;

// Конструкторы
GameInfo_t* createGameInfo();
int** createGameField(const int rows, const int cols);

// Деструкторы
void destroyGameInfo(GameInfo_t* game);
void destroyGameField(int** field);

// Game mechanics
// void updateCurrentState(GameInfo_t *curGame);
// void updateGame(GameInfo_t *curGame);
int getCellValue(int** gameboard, int col, int row);
int setCellValue(int** gameboard, int col, int row, int val);
int rotateTetramino(TetraminoState_t* tetSate, const int direction);
int moveTetramino(GameInfo_t* gameinfo, TetraminoState_t* tetState,
                  const int direction);
int checkCollision(GameInfo_t* gameinfo, TetraminoState_t* tetState);
// void userInput(UserAction_t action, bool hold); //обязательная

//Функции загрузки и выгруки лучшего результата
// int loadHighScore();
// void saveHighScore(const int hscore);

// GameInfo_t updateCurrentState(); // обязательная

#endif