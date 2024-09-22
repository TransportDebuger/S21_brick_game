#include "graphic.h"

void initGraphics(void) {
  initscr();
  raw();  //! cbrake()
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  curs_set(0);
  if (has_colors()) {
    if (start_color() == OK) {
      init_pair(1, COLOR_BLACK, COLOR_BLACK);
      init_pair(2, COLOR_BLACK, COLOR_RED);
      init_pair(3, COLOR_BLACK, COLOR_GREEN);
      init_pair(4, COLOR_BLACK, COLOR_BLUE);
      init_pair(5, COLOR_BLACK, COLOR_YELLOW);
      init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
      init_pair(7, COLOR_BLACK, COLOR_CYAN);
      init_pair(8, COLOR_BLACK, COLOR_WHITE);
    }
  }
  nodelay(stdscr, TRUE);
  scrollok(stdscr, TRUE);
}

void deinitGraphics(void) { endwin(); }

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
    for (int i = 1; i <= 9; i++) {
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