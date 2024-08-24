SHELL := /bin/bash

CC := gcc
CFLAGS := -Wall -Werror -Wextra -x c -std=c11 -pedantic
LIB_FLAGS = -lncurses
TEST_FLAGS = -lcheck
DIR_SOURCE = .
DIR_SOURCE_LIB = ./brick_game/tetris
DIR_SOURCE_GUI = ./gui/cli
DIR_HEADERS = .
DIR_HEADERS_LIB = ./brick_game/tetris
DIR_HEADERS_GUI = ./gui/cli
SOURCES = $(wildcard ${DIR_SOURCE}/*.c)
SOURCES_LIB = $(wildcard ${DIR_SOURCE_LIB}/*.c)
SOURCES_GUI = $(wildcard ${DIR_SOURCE_GUI}/*.c)
HEADERS = $(wildcard ${DIR_HEADERS}/*.h)
HEADERS_LIB = $(wildcard ${DIR_HEADERS_LIB}/*.h)
HEADERS_GUI = $(wildcard ${DIR_HEADERS_GUI}/*.h)
ALL_SOURCES = ${SOURCES} ${SOURCES_LIB} ${SOURCES_GUI}
ALL_HEADERS = ${HEADERS} ${HEADERS_LIB} ${HEADERS_GUI}
EXEC = brickgame
LIB_TEST_EXEC = test
GCOV_EXEC = gcov_report
DIR_OBJ = obj
DIR_TEST_OBJ = ${DIR_OBJ}/tests
DIR_GCOV_OBJ = ${DIR_OBJ}/gcov
DIR_REPORT = report

C_STYLE = clang-format
C_STYLE_FLAGS = -n
C_STYLE_CORR_FLAG = -i
C_CHECK = cppcheck
C_CHECK_FLAGS = --enable=all --force --suppress=missingIncludeSystem --language=c --std=c11


.PHONY: all install uninstall clean dvi dist test gcov_report styletest

.DEFAULT_GOAL: all

all:

install:

uninstall:

dvi:

dist:

${LIB_TEST_EXEC}:

${GCOV_EXEC}:

${DIR_OBJ}:
	@mkdir -p $@

${DIR_TEST_OBJ}: ${DIR_OBJ}
	@mkdir -p $@

${DIR_GCOV_OBJ}: ${DIR_OBJ}
	@mkdir -p $@

${DIR_REPORT}:
	@mkdir -p $@

${C_STYLE}: ${ALL_SOURCES} ${ALL_HEADERS}
	cp ../materials/linters/.clang-format . 
	$@ ${C_STYLE_FLAGS} $^
	rm ./.clang-format

${C_CHECK}: ${ALL_SOURCES} ${ALL_HEADERS}
	$@ ${C_CHECK_FLAGS} $^

styletest: ${C_STYLE} ${C_CHECK}

format: ${ALL_SOURCES} ${ALL_HEADERS}
	${C_STYLE} ${C_STYLE_CORR_FLAG} $^ 

clean:
	@rm -rf ${DIR_REPORT}
	@rm -rf ${DIR_TEST_OBJ}
	@rm -rf ${DIR_GCOV_OBJ}
	@rm -rf ${DIR_OBJ}