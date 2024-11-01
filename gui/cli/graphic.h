#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <ncurses.h>
#include "../../brick_game/tetris/tetris.h"

// Initializator and deinitializator ncurses
void initGraphics(void);
void deinitGraphics(void);
void gameRun(void);
int enableColorMode(void);

void showSpalshScreen();
void showMainMenu();
void clear_field(int y, int x);
void showGameBoard();
void showPauseScreen();
void print_corners(int y, int x);
void print_field_boards(int y, int x);
void print_info_boards(int y, int x);
void print_next_figure_boards(int x);
void print_score_boards(int x);
void print_control_boards(int y, int x);

#endif