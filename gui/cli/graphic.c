#include "graphic.h"

//graphic initialization
void initGraphics(void) {
  initscr();
  raw();  //! cbrake()
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);
  nodelay(stdscr, TRUE);
  scrollok(stdscr, TRUE);
}

//color scheme initialization
int enableColorMode() {
  int code = ERR;
  if (has_colors()) {
    if ((code = start_color()) == OK) {
      init_pair(1, COLOR_BLUE, COLOR_BLACK);
      init_pair(2, COLOR_BLACK, COLOR_RED);
      init_pair(3, COLOR_BLACK, COLOR_GREEN);
      init_pair(4, COLOR_BLACK, COLOR_BLUE);
      init_pair(5, COLOR_BLACK, COLOR_YELLOW);
      init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
      init_pair(7, COLOR_BLACK, COLOR_CYAN);
      init_pair(8, COLOR_BLACK, COLOR_WHITE);
    }
  }

  return code;
}

//graphic deinitialization
void deinitGraphics(void) { 
  endwin(); 
}

void showSpalshScreen() {
  int termrows = 0, termcols = 0;
  getmaxyx(stdscr, termrows, termcols);
    attrset(COLOR_PAIR(1));
    mvprintw(3, 5, "S21 TETRIS GAME");
    refresh();
    napms(1000);
}

void showMainMenu() {
  int termrows = 0, termcols = 0;
  getmaxyx(stdscr, termrows, termcols);
  mvprintw(10, 5, "PRESS \"ENTER\" TO START GAME");
  mvprintw(7, 5, "PRESS \"ESC\"  TO EXIT");
  mvprintw(14, 26, "|GOOD*|");
  mvprintw(15, 26, "|*LUCK|");
}

void clear_field(int y, int x) {
  attrset(COLOR_PAIR(1));
  for (int i = 0; i < y; i++) {
    for (int j = 0; j < x * 2; j++) {
      mvaddch(1 + i, 1 + j, ' ');
    }
  }
  refresh();
}

void showGameBoard() {
  clear_field(GAME_BOARD_HEIGHT, GAME_BOARD_WIDTH * 2);
  print_corners(20, 10);
  print_field_boards(20, 10);
  print_info_boards(20, 10);
  print_next_figure_boards(10);
  print_score_boards(10);
  print_control_boards(20, 10);
  attrset(COLOR_PAIR(1));
  refresh();
}

void print_corners(int y, int x) {
  mvaddch(0, 0, ACS_ULCORNER);
  mvaddch(0, x * 2 + 1, ACS_URCORNER);
  mvaddch(y + 1, 0, ACS_LLCORNER);
  mvaddch(y + 1, x * 2 + 1, ACS_LRCORNER);

  mvaddch(1, x * 2 + 2, ACS_ULCORNER);
  mvaddch(1, x * 2 + 16, ACS_URCORNER);
  mvaddch(6, x * 2 + 2, ACS_LLCORNER);
  mvaddch(6, x * 2 + 16, ACS_LRCORNER);

  mvaddch(8, x * 2 + 2, ACS_ULCORNER);
  mvaddch(8, x * 2 + 16, ACS_URCORNER);
  mvaddch(9, x * 2 + 2, ACS_LLCORNER);
  mvaddch(9, x * 2 + 16, ACS_LRCORNER);

  mvaddch(11, x * 2 + 2, ACS_ULCORNER);
  mvaddch(11, x * 2 + 16, ACS_URCORNER);
  mvaddch(12, x * 2 + 2, ACS_LLCORNER);
  mvaddch(12, x * 2 + 16, ACS_LRCORNER);
}

void print_field_boards(int y, int x) {
  for (int i = 1; i <= x * 2; i++) {
    mvaddch(0, i, ACS_HLINE);
    mvaddch(y + 1, i, ACS_HLINE);
  }

  for (int i = 1; i <= y; i++) {
    mvaddch(i, 0, ACS_VLINE);
    mvaddch(i, x * 2 + 1, ACS_VLINE);
  }
}

void print_info_boards(int y, int x) {
  mvaddch(0, x * 2 + 17, ACS_URCORNER);
  mvaddch(y + 1, x * 2 + 17, ACS_LRCORNER);

  for (int i = x * 2 + 2; i <= x * 2 + 16; i++) {
    mvaddch(0, i, ACS_HLINE);
    mvaddch(y + 1, i, ACS_HLINE);
  }

  for (int i = 1; i <= y; i++) {
    mvaddch(i, x * 2 + 17, ACS_VLINE);
  }
}

void print_next_figure_boards(int x) {
  for (int i = x * 2 + 3; i < x * 2 + 16; i++) {
    mvaddch(1, i, ACS_HLINE);
    mvaddch(6, i, ACS_HLINE);
  }

  for (int i = 1; i < 5; i++) {
    mvaddch(1 + i, x * 2 + 2, ACS_VLINE);
    mvaddch(1 + i, x * 2 + 16, ACS_VLINE);
  }

  mvprintw(1, x * 2 + 3, "-NEXT-FIGURE-");
}

void print_score_boards(int x) {
  mvprintw(8, x * 2 + 3, "-YOUR--SCORE-");
  mvprintw(9, x * 2 + 6, "|GOOD*|");

  for (int i = 3; i < 16; i++)
    if (i < 6 || i > 12) mvaddch(9, x * 2 + i, ACS_HLINE);

  mvprintw(12, x * 2 + 3, "-HIGH--SCORE-");
  mvprintw(11, x * 2 + 6, "|*LUCK|");

  for (int i = 3; i < 16; i++)
    if (i < 6 || i > 12) mvaddch(11, x * 2 + i, ACS_HLINE);
}

void print_field(Game_t *game) {
  // print_boards(20, 10);
  //print_level(game->gameInfo->level, 10);

  for (int i = 0; i < 20; i++)
    for (int j = 0; j < 10; j++)
      if (game->gameInfo->field[i][j])
        mvprintw(1 + i, 1 + j * 2, "[]");
      else
        mvprintw(1 + i, 1 + j * 2, "  ");

  for (int i = 0; i < 3; i++) mvprintw(3 + i, 10 * 2 + 3, "             ");
  // int n = check_rang(game->gameInfo->next);
  // for (int i = 0; i < 3; i++)
  //   for (int j = 0; j < 4; j++)
  //     if (next[i][j] == 1)
  //       mvprintw(3 + i, (x * 2 + 2) + (7 - n) + j * 2, "[]");
  //     else
  //       mvprintw(3 + i, (x * 2 + 2) + (7 - n) + j * 2, "  ");

  mvprintw(9, 10 * 2 + 7, "%d", game->gameInfo->score);
  mvprintw(11, 10 * 2 + 7, "%d", game->gameInfo->high_score);

  mvprintw(22, 1, " ROTATING - \"A\"                   ");
  mvprintw(23, 1, " MOVE DOWN - DOWN ARROW KEY         ");
  mvprintw(24, 1, " MOVE LEFT/RIGHT - L/R ARROW KEY    ");
  mvprintw(25, 1, " PAUSE - \"P\"                      ");
}

void print_control_boards(int y, int x) {
  mvaddch(y + 6, 0, ACS_LLCORNER);
  mvaddch(y + 6, x * 2 + 17, ACS_LRCORNER);

  for (int i = 1; i < 6; i++) {
    mvaddch(y + i, 0, ACS_VLINE);
    mvaddch(y + i, x * 2 + 17, ACS_VLINE);
  }

  for (int i = 1; i < x * 2 + 17; i++) {
    mvaddch(y + 6, i, ACS_HLINE);
  }
}

void showPauseScreen() {
  clear_field(20, 40);

  mvprintw(5, 5, "PRESS \"ENTER\"");
  mvprintw(6, 5, "TO  CONTINUE");

  mvprintw(11, 6, "PRESS \"ESC\"");
  mvprintw(12, 7, "TO  EXIT");
}

