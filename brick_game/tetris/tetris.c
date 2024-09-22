/*!
  \file
  \author provemet
  \version 1
  \date September 2024
  \brief Source file functions realizations for Tetris Game.

  This file contains
*/

#include <stdlib.h>
#include <stdio.h>
#include "./tetris.h"

GameInfo_t *createGameInfo() {
  GameInfo_t *game = (GameInfo_t *)malloc(sizeof(GameInfo_t));
  game->field = createGameField(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH);
  game->next = NULL;
  game->score = 0;
  game->level = 1;
  game->speed = 0;
  game->pause = 0;
  game->high_score = getHighScore();
  return game;
}

void destroyGameInfo(GameInfo_t *game) {
  if (game) {
    if (game->score > game->high_score) saveHighScore(game->score);
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

int getCellValue(const int **gameboard, int col, int row) {
  int val = -1;
  if (gameboard && col >= 0 && col < GAME_BOARD_WIDTH && row >= 0 &&
      row < GAME_BOARD_HEIGHT) {
    val = gameboard[row][col];
  }
  return val;
}

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

int rotateTetramino(TetraminoState_t *tetState,
                    const tetRotateDirection_t chdir) {
  int err = 0;

  if (tetState) {
    tetState->orientation += chdir;
    if (tetState->orientation == -1)
      tetState->orientation = ToLeft;
    else if (tetState->orientation == 4)
      tetState->orientation = ToTop;
  } else
    err = 1;

  return err;
}

int moveTetramino(TetraminoState_t *tetState,
                  const tetMoveDirection_t direction) {
  int err = 1;

  if (tetState) {
    if (direction == MoveDown) {
      tetState->offsetRow++;
    } else if (direction == MoveLeft) {
      tetState->offsetCol--;
    } else if (direction == MoveRight) {
      tetState->offsetCol++;
    } else if (direction == MoveUp) {
      tetState->offsetRow--;
    }
    err = 0;
  }

  return err;
}

int checkCollision(GameInfo_t *gameinfo, TetraminoState_t *tetState) {
  int isCollided = 0;

  if (gameinfo && tetState) {
    int tetside = Tetraminoes[tetState->tetraminoIndex].side;
    for (int i = 0; i < tetside; i++) {
      for (int j = 0; j < tetside; j++) {
        if ((tetState->orientation == ToTop &&
             getCellValue(gameinfo->field, i + tetState->offsetRow,
                          j + tetState->offsetCol) != 0 &&
             Tetraminoes[tetState->tetraminoIndex].data[i * tetside + j]) ||
            (tetState->orientation == ToRight &&
             getCellValue(gameinfo->field, i + tetState->offsetRow,
                          j + tetState->offsetCol) != 0 &&
             Tetraminoes[tetState->tetraminoIndex]
                 .data[j * tetside + (tetside - i - 1)]) ||
            (tetState->orientation == ToBottom &&
             getCellValue(gameinfo->field, i + tetState->offsetRow,
                          j + tetState->offsetCol) != 0 &&
             Tetraminoes[tetState->tetraminoIndex]
                 .data[(tetside - i - 1) * tetside + (tetside - j - 1)]) ||
            (tetState->orientation == ToLeft &&
             getCellValue(gameinfo->field, i + tetState->offsetRow,
                          j + tetState->offsetCol) != 0 &&
             Tetraminoes[tetState->tetraminoIndex]
                 .data[(tetside - j - 1) * tetside + i])) {
          isCollided = 1;
          break;
        }
      }
      if (isCollided) {
        break;
      }
    }
  }

  return isCollided;
}

int getHighScore() { 
  int hscore = 0;

  FILE* scorefile = fopen("score.dat", "r");
  if (scorefile != NULL) {
    if (fscanf(scorefile, "%d", &hscore) == 1) {
      if (hscore < 0) hscore = 0;
    }
  }
  fclose(scorefile);

  return hscore; 
}

void saveHighScore(const int hscore) {
  FILE* scorefile = fopen("score.dat", "w");
  if (scorefile != NULL) fprintf("%d", hscore);  

  fclose(scorefile);
}