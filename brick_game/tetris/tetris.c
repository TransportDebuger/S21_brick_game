/*!
  \file
  \author provemet
  \version 1
  \date September 2024
  \brief Source file functions realizations for Tetris Game.

  This file contains
*/

#include "tetris.h"

#include <stdlib.h>

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция создания структуры данных игры.
  \return Указатель на область памяти созданной структуры или NULL при
  возникновении исключений.

  Функция предназначена для создания основной структуры игры.
  В ходе выплолнения создаются структуры хранения состояния игрового поля,
  состояния фигуры, выполняется определение следующей фигуры, определяется
  первичное состояние игры.
*/
Game_t *createGame() {
  Game_t *game = NULL;

  if ((game = (Game_t *)malloc(sizeof(Game_t))) != NULL) {
    game->gameInfo = NULL;
    game->curTetState = NULL;
    game->nextTetIndex = setRandomTetraminoIndex();
    game->state = fsm_none;
    game->modified = false;
    locateGame(game);
  }

  return game;
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция очистки игровой структуры Game_t
  \param [in] game Указатель на область памяти занимаемой структурой Game_t.

  Функция предназначена для очистки данных экземпляра структуры игры.
  В ходе выплолнения данные предыдущей игры уничтожаются, структура приводится к
  состоянию по умолчанию.
*/
void clearGame(Game_t *game) {
  if (game) {
    game->state = fsm_none;
    if (game->gameInfo)
      destroyGameInfo(game->gameInfo);
    if (game->curTetState) 
      destorytetraminoState(game->curTetState);
  }
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция освобождения памяти занимаемой структурой данных игры.
  \param [in] game Указатель на область памяти занимаемой структурой Game_t.

  Функция предназначена для уничтожения экземпляра структуры игры.
  В ходе выплолнения уничтожаются освобождается память от экземпляров всех
  структур, являющихся членами данной.
*/
void destroyGame(Game_t *game) {
  if (game) {
    if (game->gameInfo != NULL) destroyGameInfo(game->gameInfo);
    if (game->curTetState != NULL) destorytetraminoState(game->curTetState);
    locateGame(game);
    free(game);
    game = NULL;
  }
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция (конструктор) выделения памяти, для структуры GameInfo_t.

  Функция предназначена для выделения области памяти для хранения структуры
  GameInfo_t предназначенной для хранения текущего состояния игры в моменте
  времени.
*/
GameInfo_t *createGameInfo() {
  GameInfo_t *gameinfo = (GameInfo_t *)malloc(sizeof(GameInfo_t));

  if (gameinfo) {
    if ((gameinfo->field =
            createGameField(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH))) {
      gameinfo->next = NULL;
      gameinfo->score = 0;
      gameinfo->level = 1;
      gameinfo->speed = 0;
      gameinfo->pause = 0;
      //gameinfo->high_score = getHighScore();
    } else
      destroyGameInfo(gameinfo);
  }

  return gameinfo;
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция (деструктор) очищения памяти, занимаемой структурой GameInfo_t.
  \param [in] gameinfo Указатель на область памяти, занимаемой структурой
  GameInfo_t.

  Функция предназначена для освобождения области памяти выделенной для хранения
  структуры GameInfo_t предназначенной для хранения текущего состояния игры в
  моменте времени.
*/
void destroyGameInfo(GameInfo_t *gameinfo) {
  if (gameinfo) {
    // if (gameinfo->score > gameinfo->high_score) {
    //   saveHighScore(gameinfo->score);
    // }
    if (gameinfo->field) 
      destroyGameField(gameinfo->field);
    free(gameinfo);
    gameinfo = NULL;
  }
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция (конструктор) создания массива ячеек игрового поля.
  \param [in] rows Целочисленное значение количества ячеек по вертикали.
  \param [in] cols Целочисленное значение количества ячеек по горизонтали.
  \return Указатель на область памяти, хранящий массив целочисленных значений
  ячеек игрового поля.

  Функция предназначена для выделения области памяти для хранения целочисленных
  значений ячеек игрового поля.
*/
int **createGameField(const int rows, const int cols) {
  int **field = (int **)calloc((size_t)rows, sizeof(int *));
  int *cells = (int *)calloc((size_t)rows * (size_t)cols, sizeof(int));
  for (int i = 0; i < rows; i++) {
    field[i] = cells + i * cols;
  }
  return field;
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция (деструктор) очищения памяти, занимаемой массивом ячеек
  игрового поля. \param [in] field Указатель на область памяти, занимаемой
  массивом значений ячеек игрового поля.

  Функция предназначена для освобождения области памяти выделенной для хранения
  целочисленных значений ячеек игрового поля.
*/
void destroyGameField(int **field) {
  if (field) {
    free(*field);
    free(field);
    field = NULL;
  }
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция инициализации структуры, хранящей значения состояния
  передвигающейся фигуры на поле. игрового поля.
*/
TetraminoState_t *createTetraminoState() {
  TetraminoState_t *tetstate =
      (TetraminoState_t *)malloc(sizeof(TetraminoState_t));
  tetstate->tetraminoIndex = 0;
  tetstate->offsetCol = 0;
  tetstate->offsetRow = 0;
  tetstate->orientation = 0;
  return tetstate;
}

/*!
  \ingroup Data_Structure_management Функции управления структурами данных
  \brief Функция (деструктор) очищения памяти, занимаемой структурой хранения
  состояния фигуры на игровом поле. игрового поля. \param [in] tetstate
  Указатель на область памяти, занимаемой структурой.

  Функция предназначена для освобождения области памяти выделенной для хранения
  состояния фигуры на игровом поле.
*/
void destorytetraminoState(TetraminoState_t *tetstate) {
  if (tetstate) {
    free(tetstate);
    tetstate = NULL;
  }
}

/*!
  \ingroup Address_providers Функции поставщиков адресов.
  \brief Функция локатор выделенных ресурсов основной игровой структуре Game_t.
  \param [in] game Указатель на адрес основной структуры игры.
  \return Указатель на область памяти, хранящую стркутру игровых данных Game_t

  \details Функции устанавливают и возвращают указатели на структуры игровых
  данных. Если адрес локатору ранее не передавался, то осуществляется сохранение
  адреса в структуре локатора. Если передан NULL и ранее локатору передавался
  адрес игровой структуры то функция возвращает значение ранее сохранненого
  адреса. При повторной передаче адреса функции локатору, состояние локатора
  изменяется на 'не задан', адрес приводится в состояние NULL.
*/
Game_t* locateGame(Game_t* game) {
  static game_locator_t location = {0};

  if (game == location.addr && location.setval) {
    location.addr = NULL;
    location.setval = nset;
  } else if (game) {
    location.addr = game;
    location.setval = set;
  }

  return location.addr;
}

// Tetramino_t** fillTatraminoes() {
//   const int i_type_tetramino[] = {0, 0, 0, 0, 1, 1, 1, 1,
//                                        0, 0, 0, 0, 0, 0, 0, 0};
//   const int o_type_tetramino[] = {2, 2, 2, 2};
//   const int t_type_tetramino[] = {0, 0, 0, 3, 3, 3, 0, 3, 0};
//   const int l_type_tetramino[] = {0, 0, 0, 4, 4, 4, 4, 0, 0};
//   const int j_type_tetramino[] = {0, 0, 0, 5, 5, 5, 0, 0, 5};
//   const int s_type_tetramino[] = {0, 0, 0, 0, 6, 6, 6, 6, 0};
//   const int z_type_tetramino[] = {0, 0, 0, 7, 7, 0, 0, 7, 7};

//   Tetramino_t** tetraminoes = (Tetramino_t **)malloc(sizeof(Tetramino_t*) *
//   TET_COUNT); if (tetraminoes) {
//     tetraminoes[0] = {i_type_tetramino, 4};
//     tetraminoes[1] = {o_type_tetramino, 2};
//     tetraminoes[2] = {t_type_tetramino, 3};
//     tetraminoes[3] = {l_type_tetramino, 3};
//     tetraminoes[4] = {j_type_tetramino, 3};
//     tetraminoes[5] = {s_type_tetramino, 3};
//     tetraminoes[6] = {z_type_tetramino, 3};
//   }

//   return tetraminoes;
// }

// void updateGameState(GameInfo_t *game){
//   if (game->pause) {

//   } else {

//   }
// };

/*!
    \ingroup Data_manipulation Функции чтения и изменения данных
    \brief Получение значения хранимого в ячейке игрового поля
    \param [in] gameboard Указатель на область памяти, хранящую массив
   значений ячеек игрового поля. \param [in] colindex Целочисленное значение,
   определяющее координату столбца целевой ячейки игрового поля. \param [in]
   rowindex Целочисленное значение, определяющее координату строки целевой
   ячейки игрового поля. \return Значение, хранимой в целевой ячейке игрового
   поля. \exception Если не передан указатель на область памяти, хранящую набор
   ячеек игрового поля возвращает -1.

     Функция предназначена для получения значения в ячейке игрового поля и
   передачи его для последующей обработки.
*/
int getCellValue(const int **gameboard, int colindex, int rowindex) {
  int val = -1;
  if (gameboard && colindex >= 0 && colindex < GAME_BOARD_WIDTH &&
      rowindex >= 0 && rowindex < GAME_BOARD_HEIGHT) {
    val = gameboard[rowindex][colindex];
  }
  return val;
}

/*!
    \ingroup Data_manipulation Функции чтения и изменения данных
     \brief Изменение значения ячейки на игровом поле.
     \param [in,out] gameboard Указатель на область памяти, хранящее массив
   значений ячеек игрового поля. \param [in] colindex Целочисленное значение,
   определяющее координату столбца целевой ячейки игрового поля. \param [in]
   rowindex елочисленное значение, определяющее координату строки целевой ячейки
   игрового поля. \param [in] val Целочисленное значение, присваиваемое ячейке
   \return 0 - успешное выполнение изменений, 1 - ошибка выполнения.
   \exception Если указатель на указатель области памяти, хранящую массив
   значений игрового поля, не передан, возвращает ненулевое значение кода
   ошибки.

     Функция предназначена для изменения значения ячейки игрового поля.
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

/*!
  \ingroup Data_manipulation Функции чтения и изменения данных
  \brief Функция выбора индекса случайной фигуры.
  \return Произвольное целое в интевале между 0 и TET_COUNT
*/
inline int setRandomTetraminoIndex() { return rand() % TET_COUNT; }

int rotateTetramino(TetraminoState_t *tetState,
                    const tetRotateDirection_t chdir) {
  int err = 1;

  if (tetState) {
    tetState->orientation = abs((tetState->orientation + chdir) % 4);
  } else
    err = 0;

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

// int checkCollision(GameInfo_t *gameinfo, TetraminoState_t *tetState) {
//   int isCollided = 0;

//   if (gameinfo && tetState) {
//     int tetside = Tetraminoes[tetState->tetraminoIndex].side;
//     for (int i = 0; i < tetside; i++) {
//       for (int j = 0; j < tetside; j++) {
//         if ((tetState->orientation == ToTop &&
//              getCellValue(gameinfo->field, i + tetState->offsetRow,
//                           j + tetState->offsetCol) != 0 &&
//              Tetraminoes[tetState->tetraminoIndex].data[i * tetside + j]) ||
//             (tetState->orientation == ToRight &&
//              getCellValue(gameinfo->field, i + tetState->offsetRow,
//                           j + tetState->offsetCol) != 0 &&
//              Tetraminoes[tetState->tetraminoIndex]
//                  .data[j * tetside + (tetside - i - 1)]) ||
//             (tetState->orientation == ToBottom &&
//              getCellValue(gameinfo->field, i + tetState->offsetRow,
//                           j + tetState->offsetCol) != 0 &&
//              Tetraminoes[tetState->tetraminoIndex]
//                  .data[(tetside - i - 1) * tetside + (tetside - j - 1)]) ||
//             (tetState->orientation == ToLeft &&
//              getCellValue(gameinfo->field, i + tetState->offsetRow,
//                           j + tetState->offsetCol) != 0 &&
//              Tetraminoes[tetState->tetraminoIndex]
//                  .data[(tetside - j - 1) * tetside + i])) {
//           isCollided = 1;
//           break;
//         }
//       }
//       if (isCollided) {
//         break;
//       }
//     }
//   }

//   return isCollided;
// }

/*!
  \ingroup Functions_getting_data_structure_for_rendering Функции получения
  данных для рендера
  \brief Функции получения данных в виде структуры
  \return Структура обмена GameInfo_t
 */
GameInfo_t updateCurrentState() {
  GameInfo_t game_info = {0};
  Game_t *game_ptr = locateGame(NULL);

  if (game_ptr) {
    game_info.field = game_ptr->gameInfo->field;
    game_info.high_score = game_ptr->gameInfo->high_score;
    game_info.level = game_ptr->gameInfo->level;
    game_info.next = game_ptr->gameInfo->next;
    game_info.pause = game_ptr->gameInfo->pause;
    game_info.score = game_ptr->gameInfo->score;
    game_info.speed = game_ptr->gameInfo->speed;
  }

  return game_info;
}

/*!
  \brief Функция обработки ввода пользователя
  \param [in] action Вид действия пользователя, определенное enum UserAction_t
  \param [in] hold Пока не понял нафига
  \details Функция выполняет обработку действий пользователя, определенные в
  соответсвии с матрицей состояний машины конечных автоматов.
*/
void userInput(UserAction_t action, bool hold) {
  actfunc act = NULL;
  Game_t *game = locateGame(NULL);
  bool holdstate = hold;

  if (game) {
    holdstate = false; //пока не понял нафига. Заглушка.
    act = fsm(game->state, action);
  }

  if (holdstate) holdstate = true;

  if (act) act(game);
}

actfunc fsm(fsm_state_t state, UserAction_t userAction) {
  actfunc callfunc = NULL;
  //Матрица предварительно накидана, нужно будет уточнять по состоянию.
  actfunc fsmMatrix[8][9] = {
      // None, Start, Pause, Terminate, Left, Right, Up, Down, Action
      {NULL, start_fn, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},  // none
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, terminate_fn, NULL, NULL, NULL, NULL, NULL},
      // {spawn_fn, NULL, pause_fn, terminate_fn, NULL, NULL, NULL, NULL,
      //  NULL},  // start
      // {NULL, NULL, pause_fn, terminate_fn, NULL, NULL, NULL, NULL,
      //  NULL},  // pause
      // {NULL, NULL, pause_fn, terminate_fn, left_fn, right_fn, up_fn, down_fn,
      //  action_fn},  // spawn
      // {NULL, NULL, pause_fn, terminate_fn, left_fn, right_fn, up_fn, down_fn,
      //  action_fn},                                                 // move
      // {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},      // shift
      // {spawn_fn, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  // connect
      // {NULL, start_fn, NULL, terminate_fn, NULL, NULL, NULL, NULL,
      //  NULL}  // game_over
  };

  callfunc = fsmMatrix[(int)state][(int)userAction];

  return callfunc;
}

void start_fn(Game_t* game) {
  if (game) {
    if ((game->gameInfo = createGameInfo())) {
      if ((game->curTetState = createTetraminoState())) {
        game->nextTetIndex = setRandomTetraminoIndex();
        game->state = fsm_start;
      } else {
        destroyGameInfo(game->gameInfo);
      }
    }
  }
}

void terminate_fn(Game_t* game) {
  if (game) {
    game->state = fsm_exit;
  }
}

void pause_fn(Game_t* game) {
  if (game) {
    static fsm_state_t temp_state;
    if (game->gameInfo->pause) {
      game->state = temp_state;
      game->gameInfo->pause = 0;
    } else {
      temp_state = game->state;
      game->gameInfo->pause = 1;
      game->state = fsm_pause;
    }
  }
}

void spawn_fn(Game_t* game) {
  if (game) {
    game->curTetState->tetraminoIndex = game->nextTetIndex;
    //Добавить задание смещения относительно центра с учетом размера фигуры.
    game->nextTetIndex = setRandomTetraminoIndex();
    game->state = fsm_none;
  }
}

// void move_fn(Game_t* game);
// void shift_fn(Game_t* game);
// void connect_fn(Game_t* game);
//void endGame_fn(Game_t* game);
void action_fn(Game_t* game) {
  if (game) return;
}

void left_fn(Game_t* game) {
  if (game) {
    moveTetramino(game->curTetState, MoveLeft);
  //   if (checkCollision(game->gameInfo, game->curTetState))
  //     moveTetramino(game->curTetState, MoveRight);
  // }
  } 
}

void right_fn(Game_t* game) {
  if (game) {
    moveTetramino(game->curTetState, MoveRight);
  //   if (checkCollision(game->gameInfo, game->curTetState))
  //     moveTetramino(game->curTetState, MoveLeft);
  // }
  }
}

//Функция заглушка
void up_fn(Game_t* game) { 
  if (game) {
    return;
  }
}

void down_fn(Game_t* game) {
  if (game) {
    moveTetramino(game->curTetState, MoveDown);
    // if (checkCollision(game->gameInfo, game->curTetState))
    //   moveTetramino(game->curTetState, MoveUp);
    //Добавить вызовы функций объединения с полем, Функцию передачи индекса
    //следующей фигуры в текущее состояние, функцию генрацию индекса фигуры,.
  }
}

/*!
  \ingroup Score_operations Функции обработки результата игры
  \brief Функция считывания из файла значения лучшего результата
  \return Возвращает целочисленное значение отражающее наилучший результат или 0
  (при возникновении исключений или попытке записать отрицательное значение).
  \exception При ошибке открытия файла или отсутсвии значения в файла.

  Функция предназначена для считывания значения лучшего результата из файла.
*/
// int getHighScore() {
//   int hscore = 0;

//   FILE *scorefile = fopen("score.dat", "r");
//   if (scorefile) {
//     if (fscanf(scorefile, "%d", &hscore) == 1) {
//       if (hscore < 0) hscore = 0;
//     }
//     fclose(scorefile);
//   }

//   return hscore;
// }

/*!
  \ingroup Score_operations Функции обработки результата игры
  \brief Функция считывания из файла значения лучшего результата
  \return Возвращает целочисленное значение отражающее наилучший результат

  Функция предназначена для записи значения лучшего результата в файл.
*/
// void saveHighScore(const int hscore) {
//   FILE *scorefile = fopen("score.dat", "w");
//   if (scorefile) {
//     fprintf(scorefile, "%d", hscore);
//     fclose(scorefile);
//   }
// }