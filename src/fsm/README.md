# FSM Library

Гибкая и модульная библиотека для работы с конечными автоматами (Finite State Machines) на языке C.

## Описание проекта

Библиотека FSM предоставляет набор инструментов для создания, управления и выполнения конечных автоматов. Она разработана с учётом принципов чистого кода, модульности и переиспользуемости.

**Основные возможности:**
- Определение состояний и переходов
- Обработка событий и действий
- Встроенная валидация
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

### 1. Клонирование репозитория
```bash
git clone <repository-url>
cd fsm
```

### 2. Сборка проекта
```bash
# Полная сборка (тесты, документация, установка)
make all

# Только сборка библиотеки (статическая библиотека)
make build

# Сборка динамической библиотеки
make build LIBTYPE=dynamic
```

### 3. Запуск тестов
```bash
# Все тесты (стиль, юнит-тесты, valgrind, покрытие)
make test

# Проверка стиля кодирования и статический анализ кода
make linter-test

# Только юнит-тесты
make unit-test

# Проверка утечек памяти
make mem-test

# Отчёт о покрытии кода
make gcov-report
```

### 4. Установка
```bash
# Установить в /usr/local (по умолчанию)
make install

# Установить в пользовательскую директорию
make install INSTALLDIR=$HOME/mylibs
```

### 5. Использование в проектах
```c
#include <s21_fsm.h>

// Ваш код здесь
```

Скомпилируйте с флагами:
```bash
gcc -Wall -Wextra -std=c11 your_program.c -o program -ls21_fsm -L$(INSTALLDIR)/bin -I$(INSTALLDIR)/include
```

## Структура подпроекта

```
fsm/
├── Makefile              # Конфигурация сборки
├── Doxyfile              # Конфигурация Doxygen
├── README.md             # Этот файл
├── *.c                   # Исходные файлы реализации
├── *.h                   # Заголовочные файлы
├── obj/                  # Объектные файлы (создаётся при сборке)
├── obj_cov/              # Объектные файлы для покрытия (создаётся при тестировании)
├── build/                # Выходные библиотеки и исполняемые файлы
├── docs/                 # Генерированная документация
├── tests/
│   ├── test_fsm.c       # Исходный код юнит-тестов
│   ├── build/           # Собранные тесты
│   ├── coverage/        # Отчёты о покрытии
│   └── coverage/report/ # HTML отчёт о покрытии
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
| `unit-test` | Сборка и запуск юнит-тестов |
| `mem-test` | Проверка утечек памяти через valgrind |
| `gcov-report` | Генерация отчёта о покрытии кода |
| `dvi` | Генерация документации (Doxygen) |
| `clean` | Удаление артефактов сборки |
| `help` | Справка по доступным целям |

## Сборка

### Статическая библиотека (по умолчанию)
```bash
make build LIBTYPE=static
# или
make build
```
Результат: `build/libs21_fsm.a`

### Динамическая библиотека
```bash
make build LIBTYPE=dynamic
```
Результат: `build/libs21_fsm.so`

## Тестирование

### Полное тестирование (рекомендуется)
```bash
make test
```

Включает:
1. **Проверка стиля** (`clang-format`)
2. **Статический анализ** (`cppcheck`)
3. **Юнит-тесты** (`check` framework)
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
## Установка и линковка

### Глобальная установка
```bash
make install
# Библиотека установлена в /usr/local/lib
# Заголовки в /usr/local/include
```

### Локальная установка
```bash
make install INSTALLDIR=$(HOME)/myapps
# Библиотека в $(HOME)/myapps/bin
# Заголовки в $(HOME)/myapps/include
```

### Использование в проекте
```bash
gcc -Wall -Wextra -std=c11 \
    -I/usr/local/include \
    -L/usr/local/lib \
    my_program.c -o my_program \
    -ls21_fsm -lm
```

## Поддерживаемые платформы

- **Linux** (Ubuntu, Debian, Fedora, RHEL, Arch)
- **macOS** (Intel и Apple Silicon)

## Стиль кода

Проект следует **Google C Style Guide** и проверяется с помощью:
- **clang-format** - для стиля форматирования
- **cppcheck** - для статического анализа

## Порядок выполнения при `make all`

1. Проверка окружения (gcc, make)
2. Создание необходимых директорий
3. Проверка зависимостей инструментов
4. **Юнит-тесты:**
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
   - Проверка окружения (doxygen, graphviz)
   - Генерация Doxygen документации
9. **Установка:**
   - Копирование библиотеки в INSTALLDIR/bin
   - Копирование заголовков в INSTALLDIR/include

## Переменные окружения

| Переменная | По умолчанию | Описание |
|------------|-------------|---------|
| `INSTALLDIR` | (зависит от системы) | Директория установки |
| `LIBTYPE` | `static` | Тип библиотеки (`static` или `dynamic`) |
| `CC` | `gcc` | C компилятор |
| `CFLAGS` | (см. Makefile) | Флаги компиляции |

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
