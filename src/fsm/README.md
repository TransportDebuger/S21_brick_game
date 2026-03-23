# FSM Library

Гибкая и модульная библиотека для работы с конечными автоматами (Finite State Machines) на языке C.

## Описание проекта

Библиотека FSM предоставляет набор инструментов для создания, управления и выполнения конечных автоматов. Она разработана с учётом принципов чистого кода, модульности и переиспользуемости. Разработана в ходе реализации серии проектов Школы 21 под общим наванием BrickGame и предполагающим реализацию игр (Тетрис, Змейка и д.р.) с использованием различных инетфейсов (CLI, Desktop, Web). По условиям проектов необходимо обеспечить совместимость и переиспользование реализованных частей в каждом отдельном проекте.

Реализация FSM в качестве отдельной библиотеки предположительно должно дать возожность не переписывать библиотеку под каждый проект и дожно обеспечить работу в проектах разработанных на других языках (С++, Python, Go, Java) через соотвествующие биндинги.

**Основные возможности:**
- Определение состояний и переходов
- Обработка событий и действий
- Встроенная валидация
- Полное покрытие юнит-тестами
- Проверка утечек памяти (valgrind)
- Генерация документации (Doxygen)

## API: ключевые компоненты

### Типы и структуры

* **`fsm_t`** — основная структура конечного автомата. Содержит текущее состояние, таблицу переходов и контекст. Не изменяйте поля вручную — используйте API.
* **`fsm_transition_t`** — структура, описывающая переход между состояниями. Содержит исходное состояние (`src`), событие (`event`), целевое состояние (`dst`) и опциональные callback-функции при входе (`on_enter`) и выходе (`on_exit`). Порядок расположения правил в массиве важен: при совпадении нескольких правил выполняется первое найденное.
* **`fsm_event_t`** — тип идентификаторов событий (триггеров) FSM. Целочисленный тип, рекомендуется использовать перечисления. Значение `0` зарезервировано под `FSM_EVENT_NONE`.
* **`fsm_state_t`** — тип идентификаторов состояний FSM. Целочисленный тип, рекомендуется использовать перечисления. Не рекомендуется использовать отрицательные значения.
* **`fsm_context_t`** — указатель на пользовательский контекст FSM. Хранит указатель на данные приложения, передаваемые в колбэки. Может быть NULL.
* **`fsm_cb_t`** — сигнатура callback-функции FSM, вызываемой при входе или выходе из состояния.

### Макросы

* **`FSM_EVENT_NONE`** — специальное событие, используемое для автоматических переходов. Передаётся как `event` в `fsm_transition_t`, чтобы обозначить переход без внешнего триггера. Используется `fsm_update()` для поиска таких переходов. Рекомендуется использовать только одно такое правило на состояние.

### Функции

* **`fsm_init`** — инициализирует конечный автомат. Принимает указатель на структуру, пользовательский контекст, массив переходов, количество переходов и начальное состояние. Возвращает `true` при успехе.
* **`fsm_destroy`** — освобождает ресурсы FSM. В текущей реализации не делает ничего, добавлен для симметрии API и будущего расширения.
* **`fsm_process_event`** — обрабатывает событие и выполняет переход. Ищет переход с `src == current` и `event == event`. Если найден — выполняет: `on_exit` → смена состояния → `on_enter`. Возвращает `true`, если переход выполнен.
* **`fsm_update`** — выполняет автоматический переход (по `FSM_EVENT_NONE`). Ищет первый переход с `src == current` и `event == FSM_EVENT_NONE`. Если найден — выполняет переход с колбэками. Используется для таймеров, условий и внутренних триггеров.

## Требования

### Обязательные зависимости:
- **GCC** - компилятор C (версия 11.0 или выше)
- **GNU Make** - система сборки

### Опциональные зависимости:
- **Check** (`libcheck-dev`) - фреймворк для юнит-тестирования
- **Valgrind** (`valgrind`) - проверка утечек памяти
- **LCOV** (`lcov`) - генерация отчётов о покрытии кода
- **Doxygen** (`doxygen`) - генерация документации
- **Graphviz** (`graphviz`) - визуализация для Doxygen
- **Clang-Format** (`clang-format`) - проверка стиля кода
- **CppCheck** (`cppcheck`) - статический анализ кода

### Установка зависимостей

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential libcheck-dev valgrind lcov doxygen graphviz clang-format cppcheck
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make libcheck-devel valgrind lcov doxygen graphviz clang-tools-extra cppcheck
```

**macOS (Homebrew):**
```bash
brew install gcc make check valgrind lcov doxygen graphviz clang-format cppcheck
```

## Сборка

### Сборка с помощью Makefile

#### Цели Makefile

| Цель | Описание |
|------|----------|
| `all` | Полная сборка (тесты → сборка → документация → установка) |
| `build` | Сборка статической/динамической библиотеки |
| `install` | Установка библиотеки и заголовков |
| `uninstall` | Удаление установленной библиотеки |
| `test` | Запуск всех тестов |
| `linter-test` | Проверка стиля кода и статический анализ |
| `unit-test` | Сборка и запуск юнит-тестов |
| `mem-test` | Проверка утечек памяти через valgrind |
| `gcov-report` | Генерация отчёта о покрытии кода |
| `dvi` | Генерация документации (Doxygen) |
| `clean` | Удаление артефактов сборки |
| `help` | Справка по доступным целям |

#### Варианты сборки с примерами команд Makefile

**Статическая библиотека (по умолчанию):**
```bash
make build LIBTYPE=static
# или просто
make build
```
Результат: `build/libs21_fsm.a`

**Динамическая библиотека:**
```bash
make build LIBTYPE=dynamic
```
Результат: `build/libs21_fsm.so`

#### Встраивание в пайплайн CI для GitHub с Makefile

```yaml
name: Build and Test FSM

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential libcheck-dev valgrind lcov doxygen graphviz clang-format cppcheck
    - name: Build and Test
      run: |
        make test
        make build
        make dvi
        make install
```

#### Встраивание в пайплайн CI для GitLab с Makefile

```yaml
stages:
  - test
  - build
  - docs

variables:
  CC: gcc
  CFLAGS: -Wall -Wextra -std=c11

before_script:
  - apt-get update
  - apt-get install -y build-essential libcheck-dev valgrind lcov doxygen graphviz clang-format cppcheck

unit-tests:
  stage: test
  script:
    - make unit-test

mem-check:
  stage: test
  script:
    - make mem-test

build-library:
  stage: build
  script:
    - make build
  artifacts:
    paths:
      - build/

generate-docs:
  stage: docs
  script:
    - make dvi
  artifacts:
    paths:
      - docs/
```

### Сборка с помощью CMake

#### Цели CMake

| Цель | Описание |
|------|----------|
| `all-build` | Полная сборка: тесты, сборка, документация, установка |
| `build` | Сборка библиотеки (LIBTYPE=static/dynamic) |
| `install_target` | Установка заголовков и библиотеки (INSTALLDIR=/path) |
| `uninstall_target` | Удаление установленной библиотеки |
| `run-tests` | Запуск всех тестов |
| `linter-test` | Проверка стиля (clang-format) и статический анализ (cppcheck) |
| `unit-test` | Сборка и запуск юнит-тестов |
| `mem-test` | Проверка утечек памяти через valgrind |
| `gcov-report` | Генерация отчёта о покрытии кода |
| `dvi` | Генерация документации (Doxygen) |
| `clean_build` | Удаление артефактов сборки |
| `help` | Справка по доступным целям |

#### Варианты сборки с примерами команд CMake

```bash
# Создание директории сборки
mkdir build && cd build

# Сборка статической библиотеки
cmake -S .. -B . -D LIBTYPE=static
cmake --build . --target build

# Сборка динамической библиотеки
cmake -S .. -B . -D LIBTYPE=dynamic
cmake --build . --target build

# Полная сборка (тесты, документация, установка)
cmake --build . --target all-build
```

#### Встраивание в пайплайн CI для GitHub с CMake

```yaml
name: CMake Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential libcheck-dev valgrind lcov doxygen graphviz clang-format cppcheck
    - name: Configure with CMake
      run: cmake -S . -B build
    - name: Build
      run: cmake --build build --target all-build
```

#### Встраивание в пайплайн CI для GitLab с CMake

```yaml
default:
  image: ubuntu:20.04

stages:
  - build
  - test

variables:
  CMAKE_BUILD_TYPE: Release

before_script:
  - apt-get update
  - apt-get install -y cmake gcc g++ make libcheck-dev valgrind lcov doxygen graphviz clang-format cppcheck

cmake-configure:
  stage: build
  script:
    - mkdir build
    - cmake -S . -B build

build-project:
  stage: build
  script:
    - cmake --build build --target all-build

run-tests:
  stage: test
  script:
    - cd build
    - ctest --output-on-failure
```

## Тестирование

### Состав тестов и особенности
Тесты расположены в директории `tests/` и охватывают все функции библиотеки. Комплекс тестирования включает:

* **Linter-тесты** (`linter-test`): Проверяют соответствие кода стилю Google C Style Guide с помощью `clang-format`, а также проводят статический анализ с помощью `cppcheck`.
* **Юнит-тесты** (`unit-test`): Основной компонент тестирования, реализованный в `test_fsm.c` с использованием фреймворка Check. Проверяются все функции библиотеки, включая корректные сценарии и edge cases (null-указатели, невалидные переходы, рекурсивные вызовы и т.д.). Тесты гарантируют правильность работы callback-функций и автоматических переходов.
* **Тесты на утечки памяти** (`mem-test`): Запускаются с помощью `valgrind` для детектирования утечек памяти, ошибок использования памяти и других проблем, связанных с управлением памятью.
* **Отчёты о покрытии кода** (`gcov-report`): Генерируются с помощью `lcov` и `genhtml` на основе данных сборки с флагами `--coverage -g`. Отчёт в формате HTML показывает, какой процент кода покрыт юнит-тестами, что позволяет оценить качество тестового покрытия.

### Необходимые зависимости для тестирования
- **Check** (`libcheck-dev`) — фреймворк юнит-тестирования
- **Valgrind** (`valgrind`) — проверка утечек памяти
- **LCOV** (`lcov`) — генерация отчётов о покрытии кода
- **Clang-Format** (`clang-format`) — проверка стиля кода
- **CppCheck** (`cppcheck`) — статический анализ кода

### Тестирование с Makefile

#### Цели Makefile для тестирования

| Цель | Описание |
|------|----------|
| `test` | Запуск всех тестов (стиль, юнит-тесты, valgrind, покрытие) |
| `linter-test` | Проверка стиля кода и статический анализ |
| `unit-test` | Сборка и запуск юнит-тестов |
| `mem-test` | Проверка утечек памяти через valgrind |
| `gcov-report` | Генерация отчёта о покрытии кода |

#### Встраивание в пайплайн CI

**GitHub Actions:**
```yaml
- name: Run all tests
  run: make test
```

**GitLab CI:**
```yaml
unit-tests:
  stage: test
  script:
    - make unit-test

mem-check:
  stage: test
  script:
    - make mem-test
```

### Тестирование с CMake

#### Цели CMake для тестирования

| Цель | Описание |
|------|----------|
| `run-tests` | Запуск всех тестов |
| `linter-test` | Проверка стиля и статический анализ |
| `unit-test` | Сборка и запуск юнит-тестов |
| `mem-test` | Проверка утечек памяти |
| `gcov-report` | Генерация отчёта о покрытии кода |

#### Встраивание в пайплайн CI

**GitHub Actions:**
```yaml
- name: Configure
  run: cmake -S . -B build
- name: Run tests
  run: cmake --build build --target run-tests
```

**GitLab CI:**
```yaml
run-tests:
  stage: test
  script:
    - cd build
    - ctest --output-on-failure
```

## Установка и линковка

### Пример глобальной установки с Makefile
```bash
make install
# Библиотека установлена в ../build/lib/bin
# Заголовки в ../build/lib/include
```

### Пример глобальной установки с CMake
```bash
cmake -S . -B build -D INSTALLDIR=/usr/local
cmake --build build --target install_target
```

### Пример локальной установки с Makefile
```bash
make install INSTALLDIR=$(HOME)/mylibs
# Библиотека в $(HOME)/mylibs/bin
# Заголовки в $(HOME)/mylibs/include
```

### Пример локальной установки с CMake
```bash
cmake -S . -B build -D INSTALLDIR=$HOME/mylibs
cmake --build build --target install_target
```

### Пример сборки и установки для дальнейшей линковки из вышестоящего Makefile
```bash
# Клонирование и установка библиотеки FSM
fsm-library:
	git clone https://github.com/S21_brick_game.git
	cd S21_brick_game/src/fsm && make build && make install INSTALLDIR=$(PWD)/external

# Флаги для компиляции и линковки
CFLAGS += -I$(PWD)/external/include
LDFLAGS += -L$(PWD)/external/bin -ls21_fsm
```

### Пример сборки и установки для дальнейшей линковки из вышестоящего CMake
```cmake
# Цель для клонирования и установки библиотеки FSM
add_custom_target(fsm-library
    COMMAND git clone https://github.com/S21_brick_game.git
    COMMAND cd S21_brick_game/src/fsm && cmake -S . -B build && cmake --build build --target all-build && cmake --build build --target install_target INSTALLDIR=${CMAKE_CURRENT_SOURCE_DIR}/external
    COMMENT "Cloning and installing FSM library..."
    VERBATIM
)

# Настройка путей и линковки
set(FSM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/include)
set(FSM_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/bin)
set(FSM_LIBRARIES s21_fsm)

include_directories(${FSM_INCLUDE_DIRS})
link_directories(${FSM_LIBRARY_DIRS})
target_link_libraries(your_target ${FSM_LIBRARIES})
```

### Пример использования в проекте в команде gcc
```bash
gcc -Wall -Wextra -std=c11 \
    -I/usr/local/include \
    -L/usr/local/lib \
    my_program.c -o my_program \
    -ls21_fsm -lm
```

## Документация

### Требования для генерации документации
Для генерации документации с помощью Doxygen требуются следующие зависимости:
- **Doxygen** (`doxygen`) — генерация документации
- **Graphviz** (`graphviz`) — визуализация диаграмм в документации

### Генерация документации с Makefile
```bash
# Сборка документации
make dvi
```

### Встраивание в пайплайн CI для GitHub с Makefile
```yaml
- name: Generate Documentation
  run: make dvi
  # Документация будет доступна в директории docs/
```

### Встраивание в пайплайн CI для GitLab с Makefile
```yaml
generate-docs:
  stage: docs
  script:
    - make dvi
  artifacts:
    paths:
      - docs/
```

### Генерация документации с CMake
```bash
# Генерация документации
cmake --build build --target dvi
```

### Встраивание в пайплайн CI для GitHub с CMake
```yaml
- name: Generate Documentation
  run: |
    cmake -S . -B build
    cmake --build build --target dvi
  # Документация будет доступна в директории docs/
```

### Встраивание в пайплайн CI для GitLab с CMake
```yaml
generate-docs:
  stage: docs
  script:
    - mkdir build
    - cmake -S . -B build
    - cmake --build build --target dvi
  artifacts:
    paths:
      - build/docs/
```

## Очистка

### Для Makefile
```bash
make clean
```

### Для CMake
```bash
cmake --build build --target clean_build
```

## Описание порядка выполнения

### при make all
1. Проверка окружения (gcc, make)
2. Создание необходимых директорий
3. Проверка зависимостей инструментов
4. **Линтинг:**
   - Проверка стиля кода (clang-format)
   - Статический анализ (cppcheck)
5. **Юнит-тесты:**
   - Компиляция с флагами покрытия (--coverage -g)
   - Запуск тестов с зависимостями (-lcheck -lsubunit -lm -lrt -lpthread)
   - Проверка памяти (valgrind)
   - Генерация отчёта о покрытии (lcov/genhtml)
6. **Сборка:**
   - Компиляция исходных файлов
   - Создание библиотеки (.a или .so)
7. **Документация:**
   - Проверка окружения (doxygen, graphviz)
   - Генерация Doxygen документации
8. **Установка:**
   - Копирование библиотеки в INSTALLDIR/bin
   - Копирование заголовков в INSTALLDIR/include

### CMake
Порядок выполнения при `cmake --build . --target all-build` аналогичен `make all`, но управляется CMake.

## Описание переменных окружения

| Переменная | По умолчанию | Описание |
|------------|-------------|---------|
| `INSTALLDIR` | (зависит от системы) | Директория установки |
| `LIBTYPE` | `static` | Тип библиотеки (`static` или `dynamic`) |
| `CC` | `gcc` | C компилятор |
| `CFLAGS` | (см. Makefile) | Флаги компиляции |

## Быстрый старт

### Для Makefile
```bash
# Клонирование репозитория
git clone https://github.com/S21_brick_game.git
# Переход в директорию подпроекта
cd S21_brick_game/src/fsm
# Сборка библиотеки (статическая)
make build

# Запуск всех тестов
make test

# Установка в ../build/lib
make install
```

### Для CMake
```bash
# Клонирование репозитория
git clone https://github.com/S21_brick_game.git
# Переход в директорию подпроекта
cd S21_brick_game/src/fsm
# Создание директории сборки
mkdir build

# Конфигурация и сборка
cmake -S . -B build && cmake --build build --target all-build

# Установка
cmake --build build --target install_target
```

## Решение проблем

### Ошибка: "gcc: command not found"
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora
sudo dnf install gcc make
```

### Ошибка: "check.h: No such file" или "subunit.h: No such file"
```bash
# Ubuntu/Debian
sudo apt-get install libcheck-dev libsubunit-dev

# Fedora
sudo dnf install libcheck-devel
```

### Тесты не компилируются
Убедитесь, что установлена библиотека `check`:
```bash
pkg-config --cflags --libs check
```

### Valgrind не установлен
```bash
# Ubuntu/Debian
sudo apt-get install valgrind

# Fedora
sudo dnf install valgrind
```

## Контакты и поддержка

Для вопросов и багов используйте систему issue репозитория.

## История версий

### v1.0 (2025-12-16)
- Первый релиз
- Полная поддержка FSM
- Полное покрытие юнит-тестами
- Документация Doxygen

---


**Последнее обновление:** 2025-12-16
