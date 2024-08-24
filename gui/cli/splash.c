#include "splash.h"

void showSpalshScreen() {
  int termrows = 0, termcols = 0;
  getmaxyx(stdscr, termrows, termcols);
  if (has_colors()) {
    if (start_color() == OK) {
      init_pair(1, COLOR_YELLOW, COLOR_RED);
    }
  }
  for (int i = 1; i <= 5; i++) {
    move(termrows / 2, termcols / 2);
    attrset(COLOR_PAIR(1));
    printw("%d", i);
    refresh();
    napms(1000);
  }
}
