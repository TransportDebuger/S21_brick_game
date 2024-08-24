#include "graphic.h"

void initGraphics(void) {
  initscr();
  raw(); //! cbrake()
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  start_color();
  init_pair(1, COLOR_MAGENTA, COLOR_WHITE);
  init_pair(2, COLOR_MAGENTA, COLOR_GREEN);
  nodelay(stdscr, TRUE);
  scrollok(stdscr, TRUE);
}

void deinitGraphics(void) {
  // delwin(game_win);
  // delwin(info_win);
  endwin();
}

int drawSplash(WINDOW *win) {
  if (win) {
    wclear(win);
    int winrows = 0, wincols = 0;
    getmaxyx(win, winrows, wincols);
    if (has_colors()) {
      if (start_color() == OK) {
        init_pair(1, COLOR_YELLOW, COLOR_RED);
      }
    }
    for (int i = 1; i <= 5; i++) {
      wattron(win, COLOR_PAIR(1));
      printw("%d", i);
      refresh();
      napms(1000);
    }
  } else {
    return 0;
  }
  return 1;
}