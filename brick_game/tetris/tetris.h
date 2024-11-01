/*!
  \file
  \author provemet
  \version 1
  \date September 2024
  \brief Заголовочный файл содержащий описание макросов, перечислений, основных
  структур, определений типов, функций, переменных и констант используемых при
  реализации библиотеки игры Тетрис.

  Файл содержит:
    ...
*/

#ifndef TETRIS_H
#define TETRIS_H

#include <stdbool.h>

#include "gamepref.h"

/*!
  \brief Макрос размерности массива фигур (тетрамино)
*/
#define TET_COUNT 7

/*!
  \brief Перечисление значения флага состояния хранения адреса локатора
*/
enum is_set { nset, set };

typedef enum fsm_state_t {
  fsm_none,
  fsm_start,
  fsm_pause,
  fsm_spawn,
  fsm_move,
  fsm_shift,
  fsm_connect,
  fsm_gameover,
  fsm_exit
} fsm_state_t;

/*!
  \brief Перечисление индексов ориентации фигуры.
*/
typedef enum tetOrientation_t {
  ToTop,  ///< Ориентация соответсвующее базовому описанию фигуры.
  ToRight,  ///< Ориентация на 90 градусов по часовой стрелке от базового
            ///< описания фигуры.
  ToBottom,  ///< Ориентация на 180 градусов по часовой стрелке от базового
             ///< описания фигуры.
  ToLeft  ///< Ориентация на 90 градсов против часовой стрелки от базового
          ///< описания фигуры.
} tetOrientation_t;

/*!
  \brief Перечисление индексов направления движения фигуры.
*/
typedef enum tetMoveDirection_t {
  MoveDown = 1,  ///< Значение направления движения вниз
  MoveLeft,  ///< Значение направления движения влево.
  MoveRight,  ///< Значение направления ддвижения вправо.
  MoveUp  ///< Служебное значение, необходимое для возврата фигуры в
          ///< первоначальное положение
} tetMoveDirection_t;

/*!
  \brief Перечисление индексов фигур для определения выбора конкретной фигуры в
  массиве.
*/
typedef enum tetraminoIndex_t {
  I_TYPE,  ///< Индекс фигуры I-типа
  O_TYPE,  ///< Индекс фигуры O-типа
  T_TYPE,  ///< Индекс фигуры T-типа
  L_TYPE,  ///< Индекс фигуры L-типа
  J_TYPE,  ///< Индекс фигуры J-типа
  S_TYPE,  ///< Индекс фигуры S-типа
  Z_TYPE   ///< Индекс фигуры Z-типа
} tetraminoIndex_t;

/*!
  \brief Структура описания фигуры тетрамино.

  Структура предназначена для реализации хранения информации о фигурах
  тетрамино.
*/
typedef struct Tetramino_t {
  const int* data;  ///< Массив ячеек хранящих размещение значимых и пустых (0)
                    ///< ячеек в квадрате описывающем фигуру.
  const int side;  ///< Целочисленное значение описывающее размерность стороны
                   ///< квадрата описывающего фигуру.
} Tetramino_t;

/*!
  \brief Структура описания состояния текущей фигуры тетрамино относительно
  игрового поля.

  Структура предназначена для хранения индекса текущей игровой фигуры, координат
  смещения относительно левого-верхнего угла игрвого поля, направления
  ориентации верха фигуры относительно оси вращения.
*/
typedef struct TetraminoState_t {
  tetraminoIndex_t
      tetraminoIndex;  ///< Целочисленное значение индекса в массиве фигур.
                       ///< Индекс может определяться с использованием enum
                       ///< tetramino_index.
  int offsetRow;  ///< Целочисленное значение смещения фигуры по вертикали
                  ///< относительно левого верхнего угла игрового поля.
  int offsetCol;  ///< Целочисленное значение смещения фигуры по горизонтали
                  ///< относительно левого верхнего угла игрового поля.
  int orientation;  ///< Целочисленное значение ориентации фигуры относительно
                    ///< оси вращения. Определяется значениями перечисления
                    ///< tetOrientation_t
} TetraminoState_t;

/*!
  \brief Структура состояния игры на игровом этапе.

  Структура хранит данные состояния игры на каждом игровом такте. Предназначена
  для передачи значений для отображения игрвого процесса.
*/
typedef struct GameInfo_t {
  int** field;  ///< Указатель область памяти хранящее двумерный массив
                ///< целочисленных значений отображающих игровое поле.
  int** next;   ///< (нафига не понял)
  int score;  ///< Целочисленное значение хранящее текущее количество очков
              ///< полученное игроком во время игрового процесса.
  int high_score;  ///< Целочисленное значение хранящее наилучший роезультат,
                   ///< полученный иигроками за все время игры.
  int level;  ///< Целочисленное значение хранящее значение текущего уровня
              ///< игры. На значение свойства speed в текущей структуре.
  int speed;  ///< Целочисленное значение хранящее значения задержки между
              ///< каждым тактом игрового процесса. Изменяется в зависимости от
              ///< значения свойства level в текущей структуре.
  int pause;  ///< Целочисленное значение индикации паузы в игре.
} GameInfo_t;

/*!
  \brief Базовая структура игры.
*/
typedef struct Game_t {
  GameInfo_t* gameInfo;  ///< Указатель на область памяти где хранится структура
                         ///< состояния игры в моменте времени.
  Tetramino_t*
      tetraminoes;  ///< Указатель на массив структур описания фигур тетрамино.
  TetraminoState_t* curTetState;  ///< Указатель на область памяти где хранится
                                  ///< состояние текущей фигуры.
  int nextTetIndex;  ///< Индекс следующей фигуры в массиве.
  fsm_state_t state;  ///< Действие выполненное пользователем.
  bool modified;
} Game_t;

typedef enum UserAction_t {
  None,
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Action
} UserAction_t;

/*!
  \brief Перечисление индексов направления вращения фигуры.
*/
typedef enum tetRotateDirection_t {
  RotateCCwise = -1,  ///< Определяет вращение против часовой стрелки
  RotateCwise = 1  ///< Определяет вращение по часовой стрелке
} tetRotateDirection_t;

/*!
  \brief Структура хранения локатора адреса структуры Game_t.
*/
typedef struct game_locator {
  Game_t* addr;  ///< Указатель на структуру Game_t
  enum is_set setval;  ///< Признак наличия заданного адреса.
} game_locator_t;

/*!
  \defgroup Score_operations Функции обработки результата игры
  \brief Функции обработки результата игры

  Функции получают и обрабатывают изменения результатов игры
*/
// int getHighScore();
// void saveHighScore(const int hscore);

/*!
  \defgroup Data_Structure_management Функции управления структурами данных
  \brief Функции создают, очищают и удаляют игровые структуры
*/
Game_t* createGame();
void clearGame(Game_t* game);
void destroyGame(Game_t* game);
GameInfo_t* createGameInfo();
void destroyGameInfo(GameInfo_t* game);
void destroyGameField(int** field);
int** createGameField(const int rows, const int cols);
TetraminoState_t* createTetraminoState();
void destorytetraminoState(TetraminoState_t* tetstate);

/*!
  \defgroup Address_providers Функции поставщиков адресов.
  \brief Функции предназначенные для предоставления адресов структур данных.
*/
Game_t* locateGame(Game_t* game);

/*!
  \brief Прототип функций действия вызываемых в соответствии со схемой конечных
  состояний.
*/
typedef void (*actfunc)(Game_t*);

void userInput(UserAction_t action, bool hold);

/*!
  \defgroup Data_manipulation Функции чтения и изменения данных
  \brief Функции предназначены для получения и изменения данных
*/
Tetramino_t* fillTatraminoes();
void destroyTetraminoes();
int getCellValue(const int** gameboard, int col, int row);
int setCellValue(int** gameboard, int col, int row, int val);
int setRandomTetraminoIndex();

/*!
  \brief Функция обработки действия "Вращение фигуры"
  \param [in,out] tetState Указатель на структуру типа struct TetraminoState_t.
  \param [in] chdir Значение определяющее направление вращения
  фигуры.
  \return 0 - если функция отработала успешно, 1 - при возникновении
  исключений.
  \exception Если не передан указатель на структуру возвращает 1.

  Функция предназначена для реализации действия "Вращение фигуры". Функция
  принимает на вход указатель на область в памяти, выделенную под структуру
  struct TetraminoState_t, в которой изменяет свойство orientation. Данное
  свойство необходимо для правильного определения ориентации фигуры относительно
  центра вращения.
  В качестве значений, передаваемых для определения направления вращения
  необходимо использовать значения перечисления tetRotateDirection_t.

*/
int rotateTetramino(TetraminoState_t* tetState,
                    const tetRotateDirection_t chdir);

/*!
  \brief Функция обработки действия "Движение фигуры"
  \param [in,out] tetState Указатель на структуру типа struct TetraminoState_t.
  \param [in] direction Значение направления движения фигуры. Значения
  определены перечислением tetMoveDirection_t. \return 0 - если функция
  отработала успешно, 1 - при возникновении исключений. \exception Если не
  передан указатель на структуру

  Функция предназначена для изменения значений свойств смещения фигуры по
  горизонтали или вертикали при обработке действия "Перемещение фигуры". Функция
  принимает на вход указатель на область в памяти, выделенную под структуру
  struct TetraminoState_t, в которой изменяет свойства offsetRow и offsetCol.
  В качестве параметра направления движения функция принимает на вход значения
  определенные перечислением tetMoveDirection_t.
*/
int moveTetramino(TetraminoState_t* tetState,
                  const tetMoveDirection_t direction);

/*!
  \brief Функция проверки коллизий расположения фигуры относительно игрового
  поля. \param [in] gameinfo
  Указатель на структуру типа struct GameInfo_t. \param [in] tetState Указатель
  на структуру типа struct TetraminoState_t. \return 0 - если функция не нашла
  пересечений ненулевых ячеек, 1 - при наличии коллизий. \exception Если не
  передан указатель на структуру

  Функция предназначена для детекции коллизий при выполнении обработки действий
  предусмотренных функциями moveTetramino(), rotateTetramino(). Функция
  принимает на вход указатели на структуры struct GameInfo_t и struct
  TetraminoState_t.
*/
int checkCollision(GameInfo_t* gameinfo, TetraminoState_t* tetState);

/*!
  \defgroup Functions_getting_data_structure_for_rendering Функции получения
  данных для рендера состояния игры.
  \brief Функции получения данных для рендер сотояния игры.


  \details Функции получают и возвращают структуру данных game_info_t
*/
GameInfo_t updateCurrentState();

/*!
  \brief Функция вызова
*/
actfunc fsm(fsm_state_t state, UserAction_t userAction);
void start_fn(Game_t* game);
void pause_fn(Game_t* game);
void spawn_fn(Game_t* game);
void move_fn(Game_t* game);
void shift_fn(Game_t* game);
void connect_fn(Game_t* game);
void gameover_fn(Game_t* game);
void terminate_fn(Game_t* game);
void left_fn(Game_t* game);
void right_fn(Game_t* game);
void up_fn(Game_t* game);
void down_fn(Game_t* game);
void action_fn(Game_t* game);

#endif  // TETRIS_H