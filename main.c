/*!
  \file
  \brief Основной файл проекта tetris

  \author provemet
  \version 1
  \date September 2024
  Данный файл содержит в себе...
*/

#include "main.h"

int main() {
  // Enable ncurses
  initGraphics();

  //enable cloring
  if (enableColorMode() == OK) {
    gameRun();
  }

  //disable ncurses
  deinitGraphics();

  return 0;
};

void gameRun(void) {
  createGame();
  //проверка предусловий
  if (!locateGame(NULL)) {
    return;
  }
  
  int temp_time = 0;
  int termrows = 0, termcols = 0;
  getmaxyx(stdscr, termrows, termcols);
  while (locateGame(NULL)->state != fsm_exit) {
    if (locateGame(NULL)->state == fsm_none && !(locateGame(NULL)->gameInfo)) {
      showMainMenu();
    } else if (locateGame(NULL)->state == fsm_start) {
      clear_field(termrows, termcols);
      showGameBoard();
      locateGame(NULL)->state = fsm_spawn;
    } else if (locateGame(NULL)->state == fsm_spawn) {
      // showGameBoard();
      // showFigure();
    } else if (locateGame(NULL)->state == fsm_pause) {
      showPauseScreen();
    } else if (locateGame(NULL)->state != fsm_none) {
      temp_time += GAME_SPEED_DELAY;
    }
    UserAction_t action = {0};
    int hold = false;
    getUserInput(&action);
    userInput(action, hold);
    // if (temp_time >= GAME_SPEED_MAX_DELAY / locateGame(NULL)->gameInfo->speed && locateGame(NULL)->state != fsm_start) {
    //   //Функция смещения фигуры вниз
    //   temp_time = 0;
    // }
  }

  destroyGame(locateGame(NULL));
}

void getUserInput(UserAction_t *action) {
  int signal = wgetch(stdscr);

  if (signal == KEY_UP)
    *action = Up;
  else if (signal == KEY_DOWN)
    *action = Down;
  else if (signal == KEY_LEFT)
    *action = Left;
  else if (signal == KEY_RIGHT)
    *action = Right;
  else if (signal == ENTER_KEY)
    *action = Start;
  else if (signal == ESCAPE)
    *action = Terminate;
  else if (signal == 'P' || signal == 'p')
    *action = Pause;
  else if (signal == 'A' || signal == 'a')
    *action = Action;
}