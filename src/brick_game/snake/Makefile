SHELL := /bin/bash
UNAME_S := $(shell uname -s)

# ============================================================================
# Настройка проекта
# ============================================================================
PROJECTNAME = s21_snake
LIBTYPE ?= static
LIBNAME := lib$(PROJECTNAME)

# ============================================================================
# Настройка путей для сборки проекта
# ============================================================================
SRCDIR = $(shell pwd)
OBJDIR = $(SRCDIR)/obj
TESTSRCDIR ?= $(SRCDIR)/tests
TESTSOURCES := $(TESTSRCDIR)/test_snake.cpp
TESTBUILDDIR ?= $(TESTSRCDIR)/build
TESTEXEC ?= test_runner
BUILDDIR := $(SRCDIR)/build
DOCDIR ?= $(SRCDIR)/docs/
COVERAGEDIR := $(TESTSRCDIR)/coverage
INSTALLDIR ?= ../../build/lib
BINDIR := $(INSTALLDIR)/bin
INCDIR := $(INSTALLDIR)/include

# ============================================================================
# Файлы исходных кодов
# ============================================================================
SOURCES = $(wildcard ${SRCDIR}/*.cpp)
INCLUDES = $(wildcard ${SRCDIR}/*.h) $(wildcard ${SRCDIR}/*.hpp)
OBJECTS := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.cpp=.o)))

OBJDIR_COV := $(SRCDIR)/obj_cov
COVERAGE_OBJECTS := $(addprefix $(OBJDIR_COV)/,$(notdir $(SOURCES:.cpp=.o)))
COVERAGE_LIB := $(BUILDDIR)/$(LIBNAME)_cov.a

# ============================================================================
# Инструменты
# ============================================================================
CXX := g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic -I../../build/lib/include

ifeq ($(LIBTYPE),dynamic)
CXXFLAGS += -fPIC
else ifeq ($(LIBTYPE),static)
else
$(error Unknown library type: $(LIBTYPE). Use LIBTYPE=static or LIBTYPE=dynamic)
endif

COVERAGE_CXXFLAGS := $(CXXFLAGS) --coverage -g

ifeq ($(UNAME_S), Linux)
    TESTLIBS = -lgtest -lpthread
endif
ifeq ($(UNAME_S), Darwin)
    TESTLIBS = -lgtest -lpthread
endif

CLFORMAT := clang-format
CLFORMATFLAGS := --style=Google --dry-run

CPPCHECK := cppcheck
CPPCHECKFLAGS := --enable=all \
				 --suppress=missingInclude \
                 --language=c++ --std=c++17 \
				 --check-config \
                 --error-exitcode=1 $(SOURCES) $(INCLUDES)

VALGRIND = valgrind
VALGRINDFLAGS = -s --leak-check=full --show-leak-kinds=all --track-origins=yes

LCOV = lcov
GENHTML := genhtml

# ============================================================================
# Цвета для вывода
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

# ============================================================================
# Проверка зависимостей
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
  else
    $(error Unknown package manager)
  endif
else ifeq ($(UNAME_S),Darwin)
  PACKAGE_MANAGER := brew
  ifeq ($(shell which $(PACKAGE_MANAGER) 2>/dev/null),/usr/local/bin/brew)
    CHECK_COMMAND := brew list
    INSTALL_COMMAND := brew install
  else
    $(error Homebrew not installed)
  endif
else
  $(error Unsupported OS: $(UNAME_S))
endif

define check-dep
    if ! $(CHECK_COMMAND) $(1) > /dev/null 2>&1; then \
        $(call print-warning, Dependency $(1) not found. Installing...); \
        if $(INSTALL_COMMAND) $(1); then \
            $(call print-success, OK: $(1) installed successfully); \
        else \
            $(call print-error, Error: $(1) failed to install); \
            exit 1; \
        fi; \
    else \
        $(call print-success, OK: $(1) is installed); \
    fi
endef

# ============================================================================
# Цели
# ============================================================================
.PHONY: all install uninstall build dvi test linter-test \
    $(CPPCHECK) $(CLFORMAT) unit-test unit-test-build unit-test-run \
    check-dep-linters check-dep-unit check-dep-memtest check-dep-coverage \
    check-dep-dvi clean clean-objects clean-docs clean-test clean-build help

.DEFAULT_GOAL := build

# ============================================================================
# ОСНОВНЫЕ ЦЕЛИ
# ============================================================================

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
		$(CXX) -shared $(OBJECTS) -L$(INSTALLDIR)/bin -o $(BUILDDIR)/$(LIBNAME).so; \
	fi
	@$(call print-success, $(LIBNAME) built successfully with $(LIBTYPE) type)
	@$(call print-success, Output: $(shell realpath $(BUILDDIR)))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDES) ../common/s21_bgame.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -I../common -c $< -o $@

$(COVERAGE_LIB): $(COVERAGE_OBJECTS)
	@mkdir -p $(BUILDDIR)
	ar rc $@ $^
	ranlib $@

$(OBJDIR_COV)/%.o: $(SRCDIR)/%.cpp $(INCLUDES)
	@mkdir -p $(dir $@)
	$(CXX) $(COVERAGE_CXXFLAGS) -DSNAKE_TEST_ACCESS -I$(SRCDIR) -I../common -c $< -o $@

# ============================================================================
# ТЕСТИРОВАНИЕ
# ============================================================================

test: linter-test unit-test mem-test gcov-report

linter-test: check-dep-linters $(CLFORMAT) $(CPPCHECK)

check-dep-linters:
	@$(call print-status, Checking linter dependencies)
	@$(call check-dep, clang-format)
	@$(call check-dep, cppcheck)

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
	if [ $$fail -ne 0 ]; then exit 1; fi

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
	if [ ! -f "$(TESTSOURCES)" ]; then \
		$(call print-warning, Test sources $(TESTSOURCES) not found (tests not yet available)); \
	else \
		$(CXX) $(COVERAGE_CXXFLAGS) -DSNAKE_TEST_ACCESS -I$(SRCDIR) -I../../build/lib/include \
			-L$(BUILDDIR) -L../../build/lib/bin \
			$(TESTSOURCES) $(TESTLIBS) \
			-l$(PROJECTNAME)_cov -ls21_bgame -ls21_fsm \
			-o $(TESTBUILDDIR)/$(TESTEXEC); \
		$(call print-success, $(TESTEXEC) built at $(TESTBUILDDIR)); \
	fi

check-dep-unit:
	@$(call print-status, Checking unit test dependencies)
	@$(call check-dep, libgtest-dev)

unit-test-run:
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
	
mem-test: check-dep-memtest
	@if [ ! -f "$(TESTBUILDDIR)/$(TESTEXEC)" ]; then \
		$(call print-warning, Test runner not found); exit 1; \
	fi
	@$(call print-status, Running Valgrind memory check); \
	rm -f /tmp/valgrind.txt; \
	LD_LIBRARY_PATH=$(BUILDDIR):../common/build:$$LD_LIBRARY_PATH \
	$(VALGRIND) $(VALGRINDFLAGS) --log-file=/tmp/valgrind.txt \
	$(TESTBUILDDIR)/$(TESTEXEC) > /dev/null 2>&1; \
	RESULT=$$?; \
	grep -E "(HEAP SUMMARY|ERROR SUMMARY)" /tmp/valgrind.txt; \
	if grep -q "ERROR SUMMARY: 0 errors" /tmp/valgrind.txt && [[ $$RESULT -eq 0 ]]; then \
		$(call print-success, Valgrind memory test [PASSED]); \
		rm -f /tmp/valgrind.txt; \
	else \
		$(call print-error, --- Memory errors detected ---); \
		grep -E "(definitely lost|indirectly lost|possibly lost)" /tmp/valgrind.txt; \
		rm -f /tmp/valgrind.txt; \
		exit 1; \
	fi; \


check-dep-memtest:
	@$(call print-status, Checking memory testing dependencies)
	@$(call check-dep, valgrind)

gcov-report: check-dep-coverage
	@if [ ! -d "$(OBJDIR_COV)" ]; then \
		$(call print-warning, Coverage data not found); exit 1; \
	fi
	@$(call print-status, Generating coverage report); \
	mkdir -p $(COVERAGEDIR); \
	$(LCOV) --capture --directory $(OBJDIR_COV) --output-file $(COVERAGEDIR)/coverage.info; \
	$(GENHTML) $(COVERAGEDIR)/coverage.info --output-directory $(COVERAGEDIR)/report/; \
	$(call print-success, Coverage report: file://$(COVERAGEDIR)/report/index.html)

check-dep-coverage:
	@$(call print-status, Checking coverage reporting dependencies)
	@$(call check-dep, lcov)

# ============================================================================
# ДОКУМЕНТАЦИЯ
# ============================================================================

dvi: check-dep-dvi
	@$(call print-status, Generating documentation)
	@mkdir -p $(DOCDIR)
	@sed "s|^OUTPUT_DIRECTORY.*|OUTPUT_DIRECTORY = $(DOCDIR)|" Doxyfile > Doxyfile.tmp
	@doxygen Doxyfile.tmp
	@rm Doxyfile.tmp
	@$(call print-success, Documentation generated at $(DOCDIR))

check-dep-dvi:
	@$(call print-status, Checking documentation dependencies)
	@$(call check-dep, doxygen)

# ============================================================================
# ОЧИСТКА
# ============================================================================

clean: clean-objects clean-docs clean-test clean-build

clean-objects:
	@rm -rf $(OBJDIR) $(OBJDIR_COV)

clean-docs:
	@rm -rf $(DOCDIR)

clean-test:
	@rm -rf $(TESTBUILDDIR) $(COVERAGEDIR)

clean-build:
	@rm -rf $(BUILDDIR)

# ============================================================================
# СПРАВКА
# ============================================================================

help:
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
