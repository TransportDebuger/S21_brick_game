#ifndef MAIN_H
#define MAIN_H

#include <time.h>
#include "./brick_game/tetris/tetris.h"
#include "./gui/cli/graphic.h"


#define SPLASH_HEIGHT 20
#define SPLASH_WIDTH 40

#define ESCAPE 27
#define ENTER_KEY 10

void gameRun(void);
void getUserInput(UserAction_t *action);

#endif