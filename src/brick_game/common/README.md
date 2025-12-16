# BrickGame - Common Library

Общая библиотека утилит для платформы BrickGame с поддержкой всех мини-игр (Tetris, Snake, Racing и т.д.).

## Описание проекта

Библиотека `s21_bgame_common` предоставляет набор инструментов и утилит, используемых всеми играми на платформе BrickGame:

**Основные возможности:**
- Управление памятью для игрового поля (20x10) и превью (4x4)
- Сохранение и загрузка рекордов на диск
- Инициализация и управление структурой состояния игры
- Валидация данных (поле, действия, состояние)
- Полное покрытие юнит-тестами
- Проверка утечек памяти (valgrind)
- Генерация документации (Doxygen)

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

## Быстрый старт

### 1. Перемещение в директорию
```bash
cd brick_game/common
```

### 2. Сборка библиотеки
```bash
# Полная сборка (тесты, сборка, документация, установка)
make all

# Только сборка библиотеки
make build

# Сборка с динамической библиотекой
make build LIBTYPE=dynamic
```

### 3. Запуск тестов
```bash
# Все тесты (стиль, юнит-тесты, valgrind, покрытие)
make test

# Только юнит-тесты
make unit-test

# Проверка утечек памяти
make mem-test

# Отчёт о покрытии кода
make gcov-report
```

### 4. Установка
```bash
# Установить в директорию сборки проекта
make install INSTALLDIR=../../build/lib

# Или установить глобально (требует sudo)
sudo make install INSTALLDIR=/usr/local
```

### 5. Использование в других модулях
Другие игры (tetris, snake) автоматически линкуются против этой библиотеки через корневой Makefile.

```bash
# Из директории brick_game
make              # Полная сборка всего проекта
make test         # Тестирование всех модулей
make clean        # Очистка артефактов
```

## Структура проекта

```
common/
├── Makefile                      # Конфигурация сборки
├── Doxyfile                      # Конфигурация Doxygen
├── README.md                     # Этот файл
├── s21_bgame.h                   # Публичный API контракт
├── s21_bgame.json                # Спецификация в JSON формате
├── s21_bgame_cmn.h               # Заголовочные файлы утилит
├── s21_bgame_cmn.c               # Реализация утилит
├── obj/                          # Объектные файлы (создаётся при сборке)
├── obj_cov/                      # Объектные файлы для покрытия (создаётся при тестировании)
├── build/                        # Выходные библиотеки и исполняемые файлы
├── docs/                         # Генерированная документация (Doxygen)
└── tests/
    ├── test_brickgame_common.c  # Исходный код 39 юнит-тестов
    ├── build/                   # Собранные тесты
    ├── coverage/                # Отчёты о покрытии
    └── coverage/report/         # HTML отчёт о покрытии
```

## Цели Makefile

| Цель | Описание |
|------|----------|
| `all` | Полная сборка (тесты → сборка → документация → установка) |
| `build` | Сборка статической/динамической библиотеки |
| `install` | Установка библиотеки и заголовков |
| `uninstall` | Удаление установленной библиотеки |
| `test` | Запуск всех тестов |
| `linter-test` | Проверка стиля кода и статический анализ |
| `unit-test` | Сборка и запуск юнит-тестов (39 тестов) |
| `mem-test` | Проверка утечек памяти через valgrind |
| `gcov-report` | Генерация отчёта о покрытии кода |
| `dvi` | Генерация документации (Doxygen) |
| `clean` | Удаление артефактов сборки |
| `distclean` | Полная очистка |
| `help` | Справка по доступным целям |

## Сборка

### Статическая библиотека (по умолчанию)
```bash
make build LIBTYPE=static
```
Результат: `build/libbrickgame_common.a`

### Динамическая библиотека
```bash
make build LIBTYPE=dynamic
```
Результат: `build/libbrickgame_common.so`

### С отладочной информацией
```bash
# Измените CFLAGS в Makefile:
# CFLAGS += -g -O0 -DDEBUG
make clean build
```

## Тестирование

### Полное тестирование (рекомендуется)
```bash
make test
```

Включает:
1. **Проверка стиля** (`clang-format`)
2. **Статический анализ** (`cppcheck`)
3. **Юнит-тесты** (`check` framework) - **39 тестов**:
   - Тесты выделения памяти (поле и превью)
   - Тесты очистки памяти
   - Тесты сохранения и загрузки рекордов
   - Тесты управления GameInfo
   - Тесты валидации данных
4. **Проверка памяти** (`valgrind`)
5. **Отчёт о покрытии** (`lcov/genhtml`)

### Запуск отдельных тестов
```bash
# Только юнит-тесты
make unit-test

# Только проверка стиля
make linter-test

# Только проверка памяти
make mem-test

# Только отчёт о покрытии
make gcov-report
```

### Просмотр результатов
```bash
# HTML отчёт о покрытии кода
open tests/coverage/report/index.html

# Документация (Doxygen)
open docs/html/index.html
```

## Компоненты библиотеки

### 1. Управление памятью (поле)
- `brickgame_allocate_field()` - выделить 20x10 поле
- `brickgame_free_field()` - освободить поле
- `brickgame_clear_field()` - очистить поле

### 2. Управление памятью (превью)
- `brickgame_allocate_next()` - выделить 4x4 превью
- `brickgame_free_next()` - освободить превью
- `brickgame_clear_next()` - очистить превью

### 3. Сохранение рекордов
- `brickgame_load_high_score()` - загрузить рекорд с диска (~/.brickgame/<игра>.score)
- `brickgame_save_high_score()` - сохранить рекорд на диск
- `brickgame_get_score_path()` - получить путь к файлу рекорда

### 4. Управление GameInfo
- `brickgame_create_game_info()` - создать и инициализировать структуру
- `brickgame_destroy_game_info()` - очистить структуру

### 5. Валидация
- `brickgame_is_valid_action()` - проверить действие пользователя
- `brickgame_is_valid_field()` - проверить игровое поле
- `brickgame_is_valid_next()` - проверить превью
- `brickgame_is_valid_game_info()` - комплексная проверка состояния

Полная документация по API находится в комментариях Doxygen в файлах `s21_bgame.h` и `s21_bgame_cmn.h`.

## Установка и линковка

### Локальная установка (рекомендуется для разработки)
```bash
# В директории common
make install INSTALLDIR=../../build/lib

# Результат:
# ../../build/lib/bin/libbrickgame_common.a
# ../../build/lib/include/s21_bgame*.h
```

### Глобальная установка
```bash
sudo make install INSTALLDIR=/usr/local

# Библиотека в /usr/local/lib/bin
# Заголовки в /usr/local/lib/include
```

### Использование в другом проекте
```c
#include <s21_bgame.h>
#include <s21_bgame_cmn.h>

// Ваш код
GameInfo_t info = brickgame_create_game_info();
```

Компилируйте с флагами:
```bash
gcc -Wall -Wextra -std=c11 \
    -I/path/to/include \
    -L/path/to/lib/bin \
    your_program.c -o program \
    -lbrickgame_common -lm
```

## Файлы сохранения рекордов

Рекорды сохраняются в:
```
~/.brickgame/<имя_игры>.score
```

**Примеры:**
```
~/.brickgame/tetris.score
~/.brickgame/snake.score
~/.brickgame/racing.score
```

Директория `.brickgame` создаётся автоматически при первом сохранении рекорда.

## Поддерживаемые платформы

- **Linux** (Ubuntu, Debian, Fedora, RHEL, Arch)
- **macOS** (Intel и Apple Silicon)
- **Windows** (WSL, MSYS2, Cygwin)

## Стиль кода

Проект следует **Google C Style Guide** и проверяется с помощью:
- **clang-format** - для стиля форматирования
- **cppcheck** - для статического анализа

## Архитектура BrickGame

Библиотека `s21_bgame_common` является **фундаментом** для всей платформы:

```
┌─────────────────────────────────────────┐
│  GUI Layer (отрисовка)                  │
├─────────────────────────────────────────┤
│  Game Engines (tetris, snake, racing)   │
│  ├─ tetris/tetris.c (реализует API)     │
│  ├─ snake/snake.c                       │
│  └─ racing/racing.c                     │
├─────────────────────────────────────────┤
│  BrickGame API (s21_bgame.h)            │
│  ├─ userInput(action, hold)             │
│  └─ updateCurrentState()                │
├─────────────────────────────────────────┤
│  Common Utilities (s21_bgame_cmn.*)     │
│  ├─ Memory management                   │
│  ├─ High score persistence              │
│  ├─ Game state management               │
│  └─ Data validation                     │
└─────────────────────────────────────────┘
```

## Порядок выполнения при `make all`

1. Проверка окружения (gcc, make)
2. Создание необходимых директорий
3. Проверка зависимостей инструментов
4. **Юнит-тесты (39 тестов):**
   - Компиляция с флагами покрытия
   - Запуск тестов
   - Проверка памяти (valgrind)
   - Генерация отчёта о покрытии
5. **Линтинг:**
   - Проверка стиля кода
   - Статический анализ
6. **Сборка:**
   - Компиляция исходных файлов
   - Создание библиотеки (.a или .so)
7. **Документация:**
   - Генерация Doxygen документации
8. **Установка:**
   - Копирование библиотеки в INSTALLDIR/lib/bin
   - Копирование заголовков в INSTALLDIR/lib/include

## Переменные окружения

| Переменная | По умолчанию | Описание |
|------------|-------------|---------|
| `INSTALLDIR` | ../../build/lib | Директория установки |
| `LIBTYPE` | `static` | Тип библиотеки (`static` или `dynamic`) |
| `CC` | `gcc` | C компилятор |

## Решение проблем

### Ошибка: "gcc: command not found"
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora
sudo dnf install gcc make
```

### Ошибка: "check.h: No such file"
```bash
# Ubuntu/Debian
sudo apt-get install libcheck-dev

# Fedora
sudo dnf install libcheck-devel
```

### Тесты не запускаются
```bash
# Проверьте наличие библиотеки check
pkg-config --cflags --libs check

# Если её нет, установите:
sudo apt-get install libcheck-dev
```

### Valgrind не установлен
```bash
# Ubuntu/Debian
sudo apt-get install valgrind

# Fedora
sudo dnf install valgrind
```

## Интеграция с проектом

Сборка из корневой директории:

```bash
cd brick_game

# Полная сборка всего проекта
make

# Сборка только common
make common

# Тестирование всех модулей
make test

# Установка в build/
make install

# Очистка
make clean
```

## История версий

### v1.0 (2025-12-16)
- Первый релиз Фазы 1
- Полная реализация управления памятью
- Полная реализация сохранения рекордов
- Система валидации данных
- 39 юнит-тестов (100% покрытие)
- Полная поддержка valgrind
- Документация Doxygen

## Лицензия

[Укажите тип лицензии, например: MIT, GPL, Apache 2.0]

## Контакты и поддержка

Для вопросов и багов используйте систему issue репозитория.

---

**Последнее обновление:** 2025-12-16
