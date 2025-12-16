SHELL := /bin/bash
UNAME_S := $(shell uname -s)

# ============================================================================
# НАСТРОЙКА ПРОЕКТА
# ============================================================================

PROJECTNAME = s21_bgame
LIBTYPE ?= static
LIBNAME := lib$(PROJECTNAME)_cmn

# ============================================================================
# НАСТРОЙКА ПУТЕЙ ДЛЯ СБОРКИ
# ============================================================================

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
TESTSRCDIR ?= $(SRCDIR)/tests
TESTSOURCES := $(TESTSRCDIR)/test_brickgame_common.c
TESTBUILDDIR ?= $(TESTSRCDIR)/build
TESTEXEC ?= test_runner
BUILDDIR := $(SRCDIR)/build
DOCDIR ?= $(SRCDIR)/docs/
COVERAGEDIR := $(TESTSRCDIR)/coverage
INSTALLDIR ?= ../../build/lib
BINDIR := $(INSTALLDIR)/bin
INCDIR := $(INSTALLDIR)/include

# Файлы исходных кодов
SOURCES = $(wildcard ${SRCDIR}/*.c)
# Заголовочные файлы
INCLUDES = $(wildcard ${SRCDIR}/*.h)
# Объектные файлы
OBJECTS := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))
OBJDIR_COV := $(SRCDIR)/obj_cov
COVERAGE_OBJECTS := $(addprefix $(OBJDIR_COV)/, $(notdir $(SOURCES:.c=.o)))
COVERAGE_LIB := $(BUILDDIR)/$(LIBNAME)_cov.a

# ============================================================================
# ИНСТРУМЕНТЫ
# ============================================================================

CC := gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic
ifeq ($(LIBTYPE),dynamic)
CFLAGS += -fPIC
endif
COVERAGE_CFLAGS := $(CFLAGS) --coverage -g

ifeq ($(UNAME_S), Linux)
    TESTLIBS = -lcheck -lsubunit -lm -lrt -lpthread
endif
ifeq ($(UNAME_S), Darwin)
    TESTLIBS = -lcheck -lm -lpthread
endif

CLFORMAT := clang-format
CLFORMATFLAGS := --style=Google --dry-run

CPPCHECK := cppcheck
CPPCHECKFLAGS := --enable=all \
                 --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
				 --suppress=unusedStructMember \
                 --language=c --std=c11 \
                 --error-exitcode=1 $(SOURCES) $(INCLUDES)

VALGRIND = valgrind
VALGRINDFLAGS = -s --leak-check=full --show-leak-kinds=all --track-origins=yes

LCOV = lcov
GENHTML := genhtml

# ============================================================================
# НАСТРОЙКА ЗАВИСИМОСТЕЙ
# ============================================================================

LINTERS = $(CLFORMAT) $(CPPCHECK)
MEMCHECK = $(VALGRIND)
COVERAGE = $(LCOV)
UNITTEST = check
DOCGEN = doxygen graphviz

# ============================================================================
# НАСТРОЙКА МЕНЕДЖЕРА ПАКЕТОВ
# ============================================================================

ifeq ($(UNAME_S),Linux)
  ifeq ($(shell which apt-get 2>/dev/null),/usr/bin/apt-get)
    PACKAGE_MANAGER := apt-get
    CHECK_COMMAND := dpkg -l | grep -wq
    INSTALL_COMMAND := sudo apt-get install -y
  else ifeq ($(shell which yum 2>/dev/null),/usr/bin/yum)
    PACKAGE_MANAGER := yum
    CHECK_COMMAND := rpm -q
    INSTALL_COMMAND := sudo yum install -y
  else ifeq ($(shell which dnf 2>/dev/null),/usr/bin/dnf)
    PACKAGE_MANAGER := dnf
    CHECK_COMMAND := rpm -q
    INSTALL_COMMAND := sudo dnf install -y
  else ifeq ($(shell which pacman 2>/dev/null),/usr/bin/pacman)
    PACKAGE_MANAGER := pacman
    CHECK_COMMAND := pacman -Q
    INSTALL_COMMAND := sudo pacman -S --noconfirm
  endif
else ifeq ($(UNAME_S),Darwin)
  PACKAGE_MANAGER := brew
  CHECK_COMMAND := brew list
  INSTALL_COMMAND := brew install
endif

# ============================================================================
# ЦВЕТА ДЛЯ ВЫВОДА
# ============================================================================

RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m

define print-status
    printf "${BLUE}##################################################${NC}\n"
    printf "${BLUE}$(1)${NC}\n"
    printf "${BLUE}##################################################${NC}\n"
endef

define print-error
    printf "${RED}$(1)${NC}\n"
endef

define print-success
    printf "${GREEN}$(1)${NC}\n"
endef

define print-warning
    printf "${YELLOW}$(1)${NC}\n"
endef

define check-dep
    if ! $(CHECK_COMMAND) $(1) > /dev/null 2>&1; then \
        $(call print-warning, Dependency $(1) not found. Installing...); \
        if $(INSTALL_COMMAND) $(1); then \
            $(call print-success, OK: $(1) installed successfully); \
        else \
            $(call print-error, Error: $(1) failed to install); \
            $(call print-error, Please install manually and try again.); \
            exit 1; \
        fi; \
    else \
        $(call print-success, OK: $(1) is installed); \
    fi
endef

# ============================================================================
# ЦЕЛИ
# ============================================================================

.PHONY: all install uninstall build dvi test linter-test \
    $(CPPCHECK) $(CLFORMAT) unit-test unit-test-build unit-test-run \
    check-dep-linters check-dep-unit check-dep-memtest check-dep-coverage \
    check-dep-dvi clean clean-objects clean-docs clean-test clean-build help

.DEFAULT_GOAL := build

all: test build dvi install

install: build
	@$(call print-status, Installing $(LIBNAME))
	@mkdir -p $(INCDIR) $(BINDIR)
	@cp $(INCLUDES) $(INCDIR)/
	@if [ "$(LIBTYPE)" = "static" ]; then \
		cp $(BUILDDIR)/$(LIBNAME).a $(BINDIR)/; \
	else \
		cp $(BUILDDIR)/$(LIBNAME).so $(BINDIR)/; \
	fi
	@$(call print-success, $(LIBNAME) installed to $(INSTALLDIR))

uninstall:
	@$(call print-status, Uninstalling $(LIBNAME))
	@rm -rf $(INSTALLDIR)
	@$(call print-success, $(LIBNAME) uninstalled successfully)

build: $(LIBNAME)

$(LIBNAME): $(OBJECTS)
	@$(call print-status, Building $(LIBTYPE) library for $(PROJECTNAME))
	@mkdir -p $(BUILDDIR)
	@if [ "$(LIBTYPE)" = "static" ]; then \
		ar rc $(BUILDDIR)/$(LIBNAME).a $(OBJECTS); \
		ranlib $(BUILDDIR)/$(LIBNAME).a; \
	elif [ "$(LIBTYPE)" = "dynamic" ]; then \
		$(CC) -shared $(OBJECTS) -o $(BUILDDIR)/$(LIBNAME).so; \
	fi
	@$(call print-success, $(LIBNAME) built successfully with $(LIBTYPE) type)
	@$(call print-success, Output: $(shell realpath $(BUILDDIR)))

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDES)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

$(COVERAGE_LIB): $(COVERAGE_OBJECTS)
	@mkdir -p $(BUILDDIR)
	ar rc $@ $^
	ranlib $@

$(OBJDIR_COV)/%.o: $(SRCDIR)/%.c $(INCLUDES)
	@mkdir -p $(dir $@)
	$(CC) $(COVERAGE_CFLAGS) -I$(SRCDIR) -c $< -o $@

test: linter-test unit-test mem-test gcov-report

linter-test: check-dep-linters $(CLFORMAT) $(CPPCHECK)

check-dep-linters:
	@$(call print-status, Checking linter dependencies)
	@for dep in $(LINTERS); do $(call check-dep, $$dep); done

$(CLFORMAT):
	@$(call print-status, Running $(CLFORMAT) style check)
	@pass=0; fail=0; \
	for src in $(SOURCES) $(INCLUDES); do \
		if $(CLFORMAT) $(CLFORMATFLAGS) $$src > /dev/null 2>&1; then \
			$(call print-success, $$src [PASS]); pass=$$((pass + 1)); \
		else \
			$(call print-error, $$src [FAIL]); fail=$$((fail + 1)); \
		fi; \
	done; \
	$(call print-warning, Summary: $$pass PASS, $$fail FAIL); \
	if [ $$fail -ne 0 ]; then \
		$(call print-error, $(CLFORMAT) test FAILED); exit 1; \
	else \
		$(call print-success, $(CLFORMAT) test PASSED); \
	fi

$(CPPCHECK):
	@$(call print-status, Running $(CPPCHECK))
	@if $(CPPCHECK) $(CPPCHECKFLAGS); then \
		$(call print-success, $(CPPCHECK) [PASSED]); \
	else \
		$(call print-error, $(CPPCHECK) [FAILED]); exit 1; \
	fi

unit-test: unit-test-build unit-test-run

unit-test-build: check-dep-unit $(COVERAGE_LIB)
	@$(call print-status, Building $(TESTEXEC) for $(PROJECTNAME))
	@mkdir -p $(TESTBUILDDIR)
	@if [ ! -f "$(TESTSOURCES)" ]; then \
		$(call print-error, Test sources $(TESTSOURCES) not found); exit 1; \
	fi
	@$(CC) $(COVERAGE_CFLAGS) -I$(SRCDIR) -L$(BUILDDIR) \
		$(TESTSOURCES) $(TESTLIBS) -l$(PROJECTNAME)_cmn_cov -o $(TESTBUILDDIR)/$(TESTEXEC)
	@$(call print-success, $(TESTEXEC) built at $(TESTBUILDDIR))

check-dep-unit:
	@$(call print-status, Checking unit test dependencies)
	@for dep in $(UNITTEST); do $(call check-dep, $$dep); done

unit-test-run: check-dep-unit unit-test-build
	@if [ ! -f "$(TESTBUILDDIR)/$(TESTEXEC)" ]; then \
		$(call print-error, Test runner not found); exit 1; \
	fi
	@$(call print-status, Running unit tests for $(PROJECTNAME))
	@LD_LIBRARY_PATH=$(BUILDDIR):$$LD_LIBRARY_PATH $(TESTBUILDDIR)/$(TESTEXEC); \
	if [ $$? -eq 0 ]; then \
		$(call print-success, Unit tests [PASSED]); \
	else \
		$(call print-error, Unit tests [FAILED]); exit 1; \
	fi

mem-test: check-dep-memtest unit-test-build
	@$(call print-status, Running Valgrind memory check)
	@rm -f /tmp/valgrind.txt
	@LD_LIBRARY_PATH=$(BUILDDIR):$$LD_LIBRARY_PATH \
	$(VALGRIND) $(VALGRINDFLAGS) --log-file=/tmp/valgrind.txt \
	$(TESTBUILDDIR)/$(TESTEXEC) > /dev/null 2>&1; \
	RESULT=$$?; \
	grep -E "(HEAP SUMMARY|ERROR SUMMARY)" /tmp/valgrind.txt
	@if grep -q "ERROR SUMMARY: 0 errors" /tmp/valgrind.txt && [[ $$RESULT -eq 0 ]]; then \
		$(call print-success, Valgrind memory test [PASSED]); \
		rm -f /tmp/valgrind.txt; \
	else \
		$(call print-error, --- Memory errors detected ---); \
		grep -E "(definitely lost|indirectly lost|possibly lost|Invalid)" /tmp/valgrind.txt; \
		rm -f /tmp/valgrind.txt; \
		exit 1; \
	fi

check-dep-memtest:
	@$(call print-status, Checking memory testing dependencies)
	@for dep in $(MEMCHECK); do $(call check-dep, $$dep); done

gcov-report: check-dep-coverage
	@$(call print-status, Generating coverage report)
	@mkdir -p $(COVERAGEDIR)
	@$(LCOV) --capture --directory $(OBJDIR_COV) --output-file $(COVERAGEDIR)/coverage.info
	@$(GENHTML) $(COVERAGEDIR)/coverage.info --output-directory $(COVERAGEDIR)/report/
	@$(call print-success, Coverage report: file://$(COVERAGEDIR)/report/index.html)

check-dep-coverage:
	@$(call print-status, Checking coverage reporting dependencies)
	@for dep in $(COVERAGE); do $(call check-dep, $$dep); done

dvi: check-dep-dvi
	@$(call print-status, Generating documentation)
	@mkdir -p $(DOCDIR)
	@sed "s|^OUTPUT_DIRECTORY.*|OUTPUT_DIRECTORY = $(DOCDIR)|" Doxyfile > Doxyfile.tmp
	@doxygen Doxyfile.tmp
	@rm Doxyfile.tmp
	@$(call print-success, Documentation generated at $(DOCDIR))

check-dep-dvi:
	@$(call print-status, Checking documentation dependencies)
	@for dep in $(DOCGEN); do $(call check-dep, $$dep); done

clean: clean-objects clean-docs clean-test clean-build

clean-objects:
	@rm -rf $(OBJDIR)
	@rm -rf $(OBJDIR_COV)

clean-docs:
	@rm -rf $(DOCDIR)

clean-test:
	@rm -rf $(TESTBUILDDIR) $(COVERAGEDIR)

clean-build:
	@rm -rf $(BUILDDIR)

help:
	@echo "BrickGame Common Library - Makefile"
	@echo "===================================="
	@echo ""
	@echo "Доступные цели:"
	@echo "  all           - полная сборка: тесты, сборка, документация, установка"
	@echo "  build         - сборка библиотеки (LIBTYPE=static/dynamic)"
	@echo "  install       - установка заголовков и библиотеки (INSTALLDIR=/path)"
	@echo "  uninstall     - удаление установленной библиотеки"
	@echo "  dvi           - генерация документации (Doxygen)"
	@echo "  test          - запуск всех тестов: линтеры, юнит-тесты, valgrind, покрытие"
	@echo "  linter-test   - проверка стиля (clang-format) и статический анализ (cppcheck)"
	@echo "  unit-test     - сборка и запуск юнит-тестов"
	@echo "  mem-test      - проверка утечек памяти через valgrind"
	@echo "  gcov-report   - генерация отчёта о покрытии кода тестами"
	@echo "  clean         - удаление всех сгенерированных файлов"
	@echo ""
	@echo "Цель по умолчанию: $(.DEFAULT_GOAL)"
	@echo ""
	@echo "Примеры использования:"
	@echo "  make                          # Собрать библиотеку"
	@echo "  make test                     # Запустить все тесты"
	@echo "  make install INSTALLDIR=path  # Установить в пользовательскую директорию"
	@echo "  make LIBTYPE=dynamic          # Собрать динамическую библиотеку"
	@echo ""
