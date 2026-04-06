# ============================================================================
# Makefile для проекта BrickGame Tetris на C11
# Модули: универсальная FSM-библиотека, библиотека Tetris, CLI-интерфейс (ncurses)
# Инструменты: gcc, ncurses, Check, Doxygen, lcov, cppcheck
# Стиль: Google C Style Guide
# ============================================================================

# Каталоги
SRCDIR       := src
BUILDDIR     := obj
TESTDIR      := tests
DOCDIR       := docs
COVERAGEDIR  := coverage

# Компилятор и флаги
CC           := gcc
CFLAGS       := -std=c11 -Wall -Wextra -Wpedantic -I$(SRCDIR) -I$(SRCDIR)/brick_game/common -I$(SRCDIR)/gui/common -I$(SRCDIR)/fsm -I$(SRCDIR)/brick_game/tetris
LDFLAGS      := -lncurses -lm
TEST_LDFLAGS := -lcheck -lm -lpthread -lrt -lsubunit -lgcov

# Цели
TARGET       := tetris
DIST_ARCHIVE := $(TARGET)-src.tar.gz

# ---------------------------------------------------------------------------
# FSM-библиотека
# ---------------------------------------------------------------------------
FSM_LIBNAME      := s21_fsm
FSM_STATIC_LIB   := lib$(FSM_LIBNAME).a
FSM_SHARED_LIB   := lib$(FSM_LIBNAME).so
FSM_SRC          := $(wildcard $(SRCDIR)/fsm/*.c)
FSM_OBJS         := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(FSM_SRC))

# ---------------------------------------------------------------------------
# Snake-библиотека
# ---------------------------------------------------------------------------
SNAKE_LIBNAME    := s21_snake
SNAKE_STATIC_LIB := lib$(SNAKE_LIBNAME).a
SNAKE_SHARED_LIB := lib$(SNAKE_LIBNAME).so
SNAKE_SRC        := $(wildcard $(SRCDIR)/brick_game/snake/*.c)
SNAKE_OBJS       := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SNAKE_SRC))

# ---------------------------------------------------------------------------
# Tetris-библиотека
# ---------------------------------------------------------------------------
TETRIS_LIBNAME    := s21_tetris
TETRIS_STATIC_LIB := lib$(TETRIS_LIBNAME).a
TETRIS_SHARED_LIB := lib$(TETRIS_LIBNAME).so
TETRIS_SRC        := $(wildcard $(SRCDIR)/brick_game/tetris/*.c)
TETRIS_OBJS       := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(TETRIS_SRC))

# ---------------------------------------------------------------------------
# CLI (ncurses)
# ---------------------------------------------------------------------------
CLI_SRC          := $(wildcard $(SRCDIR)/app/main.c)
CLI_OBJS         := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(CLI_SRC))

# ---------------------------------------------------------------------------
# Unit-тесты FSM
# ---------------------------------------------------------------------------
TEST_FSM_SRC     := $(TESTDIR)/test_fsm.c
TEST_FSM_OBJ     := $(BUILDDIR)/tests/test_fsm.o
TEST_FSM_EXE     := $(BUILDDIR)/test_fsm_runner

# ---------------------------------------------------------------------------
# Общие цели .PHONY
# ---------------------------------------------------------------------------
.PHONY: all static-libs shared-libs test_fsm test clean install uninstall \
        doc gcov_report dist check-style help

# ---------------------------------------------------------------------------
# По умолчанию: сборка библиотек и исполняемого файла
# ---------------------------------------------------------------------------
all: static-libs $(TARGET)

# ---------------------------------------------------------------------------
# Компиляция объектных файлов
# ---------------------------------------------------------------------------
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# ---------------------------------------------------------------------------
# Компиляция объектных файлов для snake
# ---------------------------------------------------------------------------
$(BUILDDIR)/$(SRCDIR)/brick_game/snake/%.o: $(SRCDIR)/brick_game/snake/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@


# ---------------------------------------------------------------------------
# Сборка статических библиотек
# ---------------------------------------------------------------------------
$(BUILDDIR)/$(FSM_STATIC_LIB): $(FSM_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(BUILDDIR)/$(SNAKE_STATIC_LIB): $(SNAKE_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^


$(BUILDDIR)/$(TETRIS_STATIC_LIB): $(TETRIS_OBJS) $(BUILDDIR)/$(FSM_STATIC_LIB)
	@mkdir -p $(dir $@)
	ar rcs $@ $(TETRIS_OBJS)


static-libs: $(BUILDDIR)/$(FSM_STATIC_LIB) $(BUILDDIR)/$(TETRIS_STATIC_LIB) $(BUILDDIR)/$(SNAKE_STATIC_LIB)

# ---------------------------------------------------------------------------
# Сборка динамических библиотек
# ---------------------------------------------------------------------------
$(BUILDDIR)/$(FSM_SHARED_LIB): $(FSM_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $^


$(BUILDDIR)/$(SNAKE_SHARED_LIB): $(SNAKE_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $^


$(BUILDDIR)/$(TETRIS_SHARED_LIB): $(TETRIS_OBJS) $(BUILDDIR)/$(FSM_SHARED_LIB)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $(TETRIS_OBJS) -L$(BUILDDIR) -l$(FSM_LIBNAME)


shared-libs: $(BUILDDIR)/$(FSM_SHARED_LIB) $(BUILDDIR)/$(TETRIS_SHARED_LIB) $(BUILDDIR)/$(SNAKE_SHARED_LIB)


# ---------------------------------------------------------------------------
# Сборка исполняемого файла
# ---------------------------------------------------------------------------
$(TARGET): $(CLI_OBJS) $(BUILDDIR)/$(TETRIS_STATIC_LIB) $(BUILDDIR)/$(FSM_STATIC_LIB)
	$(CC) $(CFLAGS) -o $@ $(CLI_OBJS) \
	    -L$(BUILDDIR) -l$(TETRIS_LIBNAME) -l$(FSM_LIBNAME) $(LDFLAGS)


# ---------------------------------------------------------------------------
# Unit-тесты FSM
# ---------------------------------------------------------------------------
$(TEST_FSM_OBJ): $(TEST_FSM_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(TEST_FSM_EXE): $(TEST_FSM_OBJ) $(BUILDDIR)/$(FSM_STATIC_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(TEST_LDFLAGS)


test_fsm: $(TEST_FSM_EXE)
	@./$(TEST_FSM_EXE)

# ---------------------------------------------------------------------------
# Общий запуск всех тестов (пока только FSM)
# ---------------------------------------------------------------------------
test: test_fsm

# ---------------------------------------------------------------------------
# Отчёт о покрытии кода
# ---------------------------------------------------------------------------
gcov_report: clean
	@mkdir -p $(COVERAGEDIR)
	$(MAKE) CFLAGS="$(CFLAGS) -fprofile-arcs -ftest-coverage" \
	       TEST_LDFLAGS="$(TEST_LDFLAGS)" static-libs test_fsm
	lcov --capture --directory $(BUILDDIR) --output-file $(COVERAGEDIR)/coverage.info
	lcov --remove $(COVERAGEDIR)/coverage.info '/usr/*' --output-file $(COVERAGEDIR)/coverage.info
	lcov --remove $(COVERAGEDIR)/coverage.info '*/tests/*' --output-file $(COVERAGEDIR)/coverage.info
	genhtml $(COVERAGEDIR)/coverage.info --output-directory $(COVERAGEDIR)
	@echo "Coverage report in $(COVERAGEDIR)/index.html"

# ---------------------------------------------------------------------------
# Генерация документации Doxygen
# ---------------------------------------------------------------------------
doc:
	@mkdir -p $(DOCDIR)
	doxygen Doxyfile

# ---------------------------------------------------------------------------
# Создание дистрибутива
# ---------------------------------------------------------------------------
dist: clean
	tar --exclude-vcs --exclude='$(BUILDDIR)' --exclude='$(COVERAGEDIR)' \
	    --exclude='$(DOCDIR)' --exclude='$(TARGET)' \
	    -czf $(DIST_ARCHIVE) \
	    $(SRCDIR) $(TESTDIR) Makefile Doxyfile README.md

# ---------------------------------------------------------------------------
# Статический анализ кода
# ---------------------------------------------------------------------------
check-style:
	cppcheck --enable=all --suppress=missingIncludeSystem \
	         --language=c --std=c11 $(SRCDIR) 2>/dev/null || true

# ---------------------------------------------------------------------------
# Установка и удаление
# ---------------------------------------------------------------------------
install: $(TARGET)
	install -d /usr/local/bin
	install $(TARGET) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(TARGET)

# ---------------------------------------------------------------------------
# Очистка артефактов сборки
# ---------------------------------------------------------------------------
clean:
	rm -rf $(BUILDDIR) $(COVERAGEDIR) $(DOCDIR)/html $(TARGET)
	find . -name '*.gcno' -delete
	find . -name '*.gcda' -delete
	find . -name '*.gcov' -delete

# ---------------------------------------------------------------------------
# Справка по целям
# ---------------------------------------------------------------------------
help:
	@echo "Доступные цели:"
	@echo "  all            — сборка библиотек и бинарника"
	@echo "  static-libs    — сборка статических библиотек"
	@echo "  shared-libs    — сборка динамических библиотек"
	@echo "  test_fsm       — сборка и запуск unit-тестов FSM"
	@echo "  test           — запуск всех тестов (FSM)"
	@echo "  gcov_report    — отчёт о покрытии кода"
	@echo "  doc            — генерация документации Doxygen"
	@echo "  dist           — создание исходного архива"
	@echo "  check-style    — статический анализ cppcheck"
	@echo "  install        — установка бинарника"
	@echo "  uninstall      — удаление бинарника"
	@echo "  clean          — очистка артефактов сборки"
	@echo "  help           — эта справка"