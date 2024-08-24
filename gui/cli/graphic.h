#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <ncurses.h>

// Initializator and deinitializator ncurses
void initGraphics(void);
void deinitGraphics(void);
int drawSplash(WINDOW *win);

#endif