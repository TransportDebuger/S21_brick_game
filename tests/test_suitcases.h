#ifndef SRC_TESTS_TESTS_SUITCASES_H
#define SRC_TESTS_TESTS_SUITCASES_H

#include <check.h>
#include <limits.h>
#include <stdbool.h>

#include "../brick_game/tetris/tetris.h"


Suite *suite_gamecreate();

void run_tests();
void run_testcase(Suite *testcase);

#endif