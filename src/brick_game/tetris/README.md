# 🎮 s21_tetris — Библиотека игры Тетрис v2.2

**Полнофункциональная реализация классического Тетриса на C с интеграцией в многоигровой движок BrickGame**

---

## 📋 Содержание

- [Обзор](#обзор)
- [Ключевые особенности](#ключевые-особенности)
- [Быстрый старт](#быстрый-старт)
- [Архитектура](#архитектура)
- [API Reference](#api-reference)
- [Примеры использования](#примеры-использования)
- [Структуры данных](#структуры-данных)
- [FSM Диаграмма](#fsm-диаграмма)
- [Производительность](#производительность)
- [Тестирование](#тестирование)
- [Часто задаваемые вопросы](#часто-задаваемые-вопросы)

---

## 🎯 Обзор

**s21_tetris** — это профессиональная реализация игры Тетрис, разработанная для:

- **Классического геймплея** с семью типами фигур (тетромино)
- **Современной архитектуры** на основе конечного автомата (FSM)
- **Интеграции с движком** BrickGame для многоигровых сценариев
- **Высокого качества кода** с полной документацией Doxygen

### Основная информация

| Параметр | Значение |
|----------|----------|
| **Версия** | 2.2 |
| **Язык** | C (C99/C11 compatible) |
| **Размер поля** | 20×10 (классический стандарт) |
| **Фигуры** | 7 типов (I, O, T, S, Z, J, L) |
| **Состояния FSM** | 6 состояний + переходы |
| **Память на игру** | ~2.7 KB |
| **Лицензия** | MIT (или указать вашу) |

---

## ✨ Ключевые особенности

### 🔒 Архитектура
- ✅ **Паттерн Фасад** — чистый публичный API, скрывающий сложность
- ✅ **Инкапсуляция** — внутренняя структура TetrisGame скрыта через неполный тип
- ✅ **FSM на основе таблицы переходов** — надёжное управление состояниями
- ✅ **Разделение ответственности** — отдельные слои для логики и отрисовки

### 🎮 Игромеханика
- ✅ **7 тетромино** с правильной физикой падения
- ✅ **4 направления ротации** для каждой фигуры
- ✅ **Soft drop и hard drop** — разные скорости падения
- ✅ **Очистка линий** с подсчётом очков и повышением уровня
- ✅ **Пауза и возобновление** игры
- ✅ **Сохранение рекорда** на диск

### 🛡️ Надёжность
- ✅ **Обработка NULL-указателей** во всех функциях
- ✅ **Exception-safe выделение памяти** с откатом при ошибке
- ✅ **Проверка границ** для предотвращения buffer overflow
- ✅ **Валидация координат и типов** фигур

### 📚 Документация
- ✅ **Полная Doxygen-документация** всех функций
- ✅ **Комментарии к сложным алгоритмам**
- ✅ **Примеры использования** в исходном коде
- ✅ **Описание констант** и их назначения

---

## 🚀 Быстрый старт

### Установка

```bash
# Скопируйте файлы в ваш проект
cp s21_tetris.h s21_tetris.c s21_tetris_internals.* s21_tetris_pieces.c /path/to/your/project

# Убедитесь, что зависимости установлены
# - s21_fsm.h (модуль конечного автомата)
# - s21_bgame.h (интерфейсы игрового движка)
```

### Базовый пример

```c
#include "s21_tetris.h"

int main() {
    // 1. Создание игры
    void *game = tetris_create();
    if (!game) {
        fprintf(stderr, "Failed to create game\n");
        return 1;
    }
    
    // 2. Запуск игры
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    
    // 3. Основной игровой цикл
    while (1) {
        // Получаем состояние для отрисовки
        const GameInfo_t *info = tetris_get_info(game);
        if (!info) break;
        
        // Отрисовываем (реализация зависит от вашего движка)
        draw_game_state(info);
        
        // Проверяем окончание
        if (info->game_over) break;
        
        // Обрабатываем пользовательский ввод
        UserAction_t action = get_user_action();
        if (action != Up) {  // Up — пусто, игнорируется
            tetris_handle_input(game, action, is_key_held());
        }
        
        // Игровой тик (управляет падением)
        tetris_update(game);
        
        // Задержка для FPS контроля (~50-100ms для классического Тетриса)
        usleep(100000);
    }
    
    // 4. Очистка
    tetris_destroy(game);
    return 0;
}
```

### Компиляция

```bash
gcc -c s21_tetris.c s21_tetris_internals.c s21_tetris_pieces.c
gcc -o my_game main.c s21_tetris.o s21_tetris_internals.o s21_tetris_pieces.o \
    -L. -ls21_fsm -ls21_bgame -lm
```

---

## 🏛️ Архитектура

### Структура файлов

```
s21_tetris/
├── s21_tetris.h                 (8.5 KB)  — Публичный API
├── s21_tetris.c                 (8.3 KB)  — Реализация фасада
├── s21_tetris_internals.h       (57.9 KB) — Структуры и интерфейсы
├── s21_tetris_internals.c       (39.7 KB) — Логика игры
└── s21_tetris_pieces.c          (7.4 KB)  — Данные фигур
```

### Паттерны проектирования

#### 1. **Фасад** (Facade)
```
Пользователь → [s21_tetris.h API] → [s21_tetris.c фасад] → 
    [internals.c логика] → [internals.h структуры]
```

#### 2. **Инкапсуляция через неполный тип**
```c
// В заголовке: неполный тип (forward declaration)
struct TetrisGame;  // только объявление, без определения

// В .c файле: полное определение
struct TetrisGame {
    fsm_t fsm;
    TetrisPiece current;
    // ... остальные поля
};

// Результат: внешний код не может напрямую обращаться к полям
```

#### 3. **Таблица переходов FSM**
```c
static const fsm_transition_t tetris_transitions[] = {
    // Состояние | Событие | Новое состояние | on_enter | on_exit
    { INIT,  START,     SPAWN,      NULL,          on_enter_spawn },
    { SPAWN, TICK,      FALL,       NULL,          NULL },
    { FALL,  TICK,      LOCK,       NULL,          on_enter_lock },
    // ... и так далее
};
```

---

## 📖 API Reference

### Функции создания/уничтожения

#### `tetris_create(void)`
Создаёт новый экземпляр игры.

```c
void *game = tetris_create();
if (!game) {
    perror("Memory allocation failed");
    return -1;
}
```

**Возвращает:** Указатель на игру или NULL при ошибке.

#### `tetris_destroy(void *game)`
Освобождает ресурсы игры.

```c
tetris_destroy(game);  // Безопасно даже с NULL
```

---

### Функции ввода/обновления

#### `tetris_handle_input(void *game, UserAction_t action, bool hold)`
Обрабатывает действие пользователя.

```c
// Простое движение влево
tetris_handle_input(game, Left, false);

// Hard drop (удержание "вниз")
tetris_handle_input(game, Down, true);

// Soft drop (однократное нажатие "вниз")
tetris_handle_input(game, Down, false);

// Поворот
tetris_handle_input(game, Action, false);

// Пауза
tetris_handle_input(game, Pause, false);
```

**Параметры:**
- `action` — UserAction_t (Start, Left, Right, Down, Up, Action, Pause, Terminate)
- `hold` — true для удержания, false для одного нажатия

#### `tetris_update(void *game)`
Обновляет состояние на один тик.

```c
// Вызывается регулярно (например, раз в 100ms)
tetris_update(game);
```

**Что происходит при вызове:**
- Падение фигуры вниз на одну клетку
- Проверка коллизий
- Фиксация фигуры при необходимости
- Очистка линий и подсчёт очков

---

### Функция получения состояния

#### `tetris_get_info(const void *game)`
Возвращает текущее состояние для отрисовки.

```c
const GameInfo_t *info = tetris_get_info(game);
if (!info) {
    fprintf(stderr, "Invalid game pointer\n");
    return;
}

// Используем info для отрисовки
printf("Score: %d\n", info->score);
printf("Level: %d\n", info->level);
printf("Game Over: %s\n", info->game_over ? "Yes" : "No");
```

**Содержимое GameInfo_t:**
```c
struct GameInfo_t {
    int **field;        // Игровое поле [20][10]
    int **next;         // Следующая фигура [4][4]
    int score;          // Текущий счёт
    int high_score;     // Рекорд
    int level;          // Уровень (1, 2, 3, ...)
    int speed;          // Задержка в мс
    int pause;          // Флаг паузы (0/1)
    int game_over;      // Флаг окончания (0/1)
};
```

---

### Функция регистрации

#### `tetris_get_interface(GameId_t id)`
Возвращает интерфейс для регистрации в движке.

```c
GameInterface_t iface = tetris_get_interface(GAME_TETRIS);

// Теперь движок может вызывать функции игры:
void *game = iface.create();
iface.input(game, Left, false);
iface.update(game);
const GameInfo_t *info = iface.get_info(game);
iface.destroy(game);
```

---

## 📚 Структуры данных

### TetrisGame (основная структура)

Скрыта в `s21_tetris_internals.c`. Содержит:

```c
struct TetrisGame {
    fsm_t fsm;                      // Конечный автомат
    TetrisPiece current;            // Текущая активная фигура
    TetrisPiece next;               // Следующая фигура
    int **field_storage;            // Основное поле [20][10]
    int **next_storage;             // Буфер следующей [4][4]
    GameInfo_t info;                // Публичное состояние
    int game_over;                  // Флаг окончания
    int score;                      // Текущий счёт
    int level;                      // Уровень
    int lines_cleared;              // Очищенные линии
    int high_score;                 // Рекорд
    int speed;                      // Текущая скорость
    // ... и другие поля
};
```

### TetrisPiece (фигура)

```c
struct TetrisPiece {
    int type;       // 0-6 (I, O, T, S, Z, J, L)
    int rotation;   // 0-3 (0°, 90°, 180°, 270°)
    int x;          // Позиция X в поле
    int y;          // Позиция Y в поле
};
```

### TETROMINO_DATA (формы фигур)

```c
// Глобальный массив форм (в s21_tetris_pieces.c)
extern const int TETROMINO_DATA[7][4][4][4];
//                                 ↑  ↑  ↑  ↑
//                    тип (7)  вращение (4)  строка (4)  столбец (4)
```

Каждая ячейка содержит:
- `0` — пустая ячейка
- `1–7` — ID фигуры (используется при отрисовке)

**Пример I-фигуры (вертикальная ориентация):**
```
0 1 0 0
0 1 0 0
0 1 0 0
0 1 0 0
```

---

## 🔄 FSM Диаграмма

### Состояния

```
┌─────────────────────────────────────────────────────────┐
│ INIT (инициализация)                                    │
│ - Очищает поле и статистику                            │
│ - Генерирует первые фигуры                             │
│ - Инициализирует FSM                                   │
└──────────────────┬──────────────────────────────────────┘
                   │ START
                   ↓
┌─────────────────────────────────────────────────────────┐
│ SPAWN (спавн новой фигуры)                              │
│ - Перемещает next в current                            │
│ - Генерирует новую next                                │
│ - Проверяет коллизию (если есть → GAME_OVER)          │
└──────────────────┬──────────────────────────────────────┘
                   │ TICK
                   ↓
┌─────────────────────────────────────────────────────────┐
│ FALL (активное падение)                                 │
│ - Обрабатывает движения (LEFT, RIGHT)                 │
│ - Обрабатывает повороты (ROTATE)                      │
│ - Обрабатывает мягкое падение (MOVE_DOWN)             │
│ - Обрабатывает жёсткое падение (DROP)                 │
│ - Проверяет коллизию при падении                      │
│ ↑                                                       │
│ │ PAUSE_TOGGLE                                         │
│ │                                                       │
└──────────┬──────────────────────┬──────────────────────┘
           │                      │
        TICK/DOWN/DROP         PAUSE_TOGGLE
           │                      │
           ↓                      ↓
  ┌──────────────┐      ┌─────────────────┐
  │ LOCK         │      │ PAUSED          │
  │ (фиксация)   │      │ (пауза)         │
  │              │      │                 │
  │ - Lock       │      │ - Ожидает       │
  │ - Check&Clear│      │   возобновления │
  │ - AddScore   │      └────┬────────────┘
  │ - UpdateLevel│            │ PAUSE_TOGGLE
  │              │            │
  └──────┬───────┘            ↓
         │ TICK           FALL ←┘
         ↓
    SPAWN (новая фигура)

┌──────────────────────────────────┐
│ GAME_OVER (конец игры)           │
│ - Отображает финальный счёт      │
│ - Сохраняет рекорд              │
│ - Ожидает нового старта          │
└──────────┬───────────────────────┘
           │ START
           ↓
        INIT
```

### Таблица переходов (14 правил)

| Текущее | Событие | Новое | Действие |
|---------|---------|-------|----------|
| INIT | START | SPAWN | Спавн первой фигуры |
| SPAWN | TICK | FALL | Начало падения |
| SPAWN | NONE | GAME_OVER | Коллизия при спавне |
| FALL | TICK | LOCK | Фиксация при падении |
| FALL | MOVE_DOWN | LOCK | Soft drop → фиксация |
| FALL | DROP | LOCK | Hard drop → фиксация |
| FALL | MOVE_LEFT | FALL | Движение влево |
| FALL | MOVE_RIGHT | FALL | Движение вправо |
| FALL | ROTATE | FALL | Поворот |
| FALL | PAUSE_TOGGLE | PAUSED | Пауза |
| PAUSED | PAUSE_TOGGLE | FALL | Возобновление |
| FALL | TERMINATE | GAME_OVER | Выход |
| LOCK | TICK | SPAWN | Спавн следующей |
| GAME_OVER | START | INIT | Перезапуск |

---

## 💡 Примеры использования

### Пример 1: Простая интеграция

```c
#include "s21_tetris.h"
#include <unistd.h>

void main_game_loop() {
    void *game = tetris_create();
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    
    while (1) {
        const GameInfo_t *info = tetris_get_info(game);
        
        // Проверяем окончание
        if (info->game_over) {
            printf("Game Over! Score: %d\n", info->score);
            break;
        }
        
        // Простая отрисовка поля
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 10; j++) {
                printf("%s", info->field[i][j] ? "██" : "  ");
            }
            printf("\n");
        }
        printf("Score: %d, Level: %d\n\n", info->score, info->level);
        
        // Автоматическое падение (без ввода)
        tetris_update(game);
        usleep(500000);  // 500ms
    }
    
    tetris_destroy(game);
}
```

### Пример 2: С обработкой ввода

```c
#include "s21_tetris.h"

int main() {
    void *game = tetris_create();
    tetris_handle_input(game, Start, false);
    
    while (1) {
        // Обработка ввода (зависит от вашей системы)
        int ch = getchar();
        
        switch (ch) {
            case 'a':  tetris_handle_input(game, Left, false); break;
            case 'd':  tetris_handle_input(game, Right, false); break;
            case 's':  tetris_handle_input(game, Down, false); break;
            case 'w':  tetris_handle_input(game, Action, false); break;  // Поворот
            case 'p':  tetris_handle_input(game, Pause, false); break;
            case 'q':  tetris_handle_input(game, Terminate, false); break;
        }
        
        // Игровой тик
        tetris_update(game);
        
        // Получаем и отображаем состояние
        const GameInfo_t *info = tetris_get_info(game);
        if (info->game_over) {
            printf("Final score: %d\n", info->score);
            break;
        }
    }
    
    tetris_destroy(game);
    return 0;
}
```

### Пример 3: Интеграция с BrickGame движком

```c
#include "s21_bgame.h"

int main() {
    GameEngine_t engine;
    game_engine_init(&engine);
    
    // Регистрируем Тетрис в движке
    GameInterface_t tetris = tetris_get_interface(GAME_TETRIS);
    game_engine_register_game(&engine, &tetris);
    
    // Переключаемся на Тетрис
    game_engine_switch_game(&engine, GAME_TETRIS);
    
    // Используем движок для управления игрой
    while (!engine.should_quit) {
        UserAction_t action = read_user_input();
        engine.input(&engine.current_game, action, is_held);
        engine.update(&engine.current_game);
        
        const GameInfo_t *info = engine.get_info(&engine.current_game);
        render_game(info);
    }
    
    game_engine_cleanup(&engine);
    return 0;
}
```

---

## 📊 Производительность

### Память

```
TetrisGame структура:        1 KB
field_storage (20×10×4):     800 bytes
next_storage (4×4×4):         64 bytes
info.field (20×10×4):        800 bytes
────────────────────────────────
ИТОГО на игру:              ~2.7 KB
```

### Временная сложность

| Операция | Сложность | Примечание |
|----------|-----------|-----------|
| tetris_create() | O(1) | Выделение памяти и инициализация |
| tetris_destroy() | O(1) | Освобождение памяти |
| tetris_handle_input() | O(1) | Преобразование действия в событие |
| tetris_move_piece() | O(1) | Проверка 16 ячеек (4×4 матрица) |
| tetris_rotate_piece() | O(1) | Проверка новой ориентации |
| tetris_check_collision() | O(1) | Проверка 16 ячеек |
| tetris_lock_piece() | O(1) | Размещение 16 ячеек |
| tetris_update() | O(200) = O(1) | Обновление поля 20×10 |
| tetris_get_info() | O(200) = O(1) | Синхронизация информации |

**Вывод:** Все операции практически мгновенны (<<1ms).

---

## 🧪 Тестирование

### Модульные тесты (примеры)

```c
#include <assert.h>
#include "s21_tetris.h"

void test_create_destroy() {
    void *game = tetris_create();
    assert(game != NULL);
    tetris_destroy(game);
    assert(true);  // No crash
}

void test_handle_input_null() {
    tetris_handle_input(NULL, Start, false);
    // Should not crash
}

void test_move_piece() {
    void *game = tetris_create();
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    tetris_update(game);  // Переходим в FALL
    
    // Попытаемся переместить фигуру
    const GameInfo_t *info = tetris_get_info(game);
    assert(info != NULL);
    
    tetris_destroy(game);
}

int main() {
    test_create_destroy();
    test_handle_input_null();
    test_move_piece();
    printf("All tests passed!\n");
    return 0;
}
```

### Интеграционные тесты

```bash
# Компилируем с тестами
gcc -g -O0 tests.c s21_tetris.c s21_tetris_internals.c s21_tetris_pieces.c -o test_tetris

# Запускаем с GDB для отладки
gdb ./test_tetris
```

---

## ❓ Часто задаваемые вопросы

### Q: Как изменить размер поля?
**A:** Измените константы в `s21_tetris_internals.h`:
```c
#define TETRIS_FIELD_ROWS 20  // Измените на нужное значение
#define TETRIS_FIELD_COLS 10
```
Затем пересоберите проект.

### Q: Как добавить новый тип фигуры?
**A:** 
1. Добавьте новый тип в `s21_tetris_pieces.c`
2. Увеличьте `TETRIS_NUM_PIECES`
3. Добавьте форму в `TETROMINO_DATA`

### Q: Как ускорить игру?
**A:** Уменьшите время между `tetris_update()` вызовами или используйте опцию `-O3` при компиляции.

### Q: Потокобезопасна ли библиотека?
**A:** **Нет.** Все функции работают с общим состоянием и требуют синхронизации при использовании из нескольких потоков.

### Q: Как сохранить состояние игры?
**A:** Рекорд сохраняется автоматически в `~/.brickgame/tetris.score`. Для полного состояния вам нужно сохранить структуру вручную.

---

## 📝 Документация и ссылки

- **Doxygen документация:** Генерируется из комментариев в исходном коде
- **Tetris Guideline:** https://tetris.fandom.com/wiki/Tetris_Guideline
- **FSM паттерны:** https://en.wikipedia.org/wiki/Finite-state_machine

---

## 🔧 Компиляция

### С поддержкой отладки
```bash
gcc -g -O0 -Wall -Wextra -std=c99 -c s21_tetris*.c s21_tetris_pieces.c
```

### С оптимизацией
```bash
gcc -O3 -Wall -std=c11 -c s21_tetris*.c s21_tetris_pieces.c
```

### С анализатором памяти
```bash
gcc -g -fsanitize=address,undefined -c s21_tetris*.c s21_tetris_pieces.c
```

---

## 📄 Лицензия

Код распространяется под лицензией **MIT** (или укажите вашу лицензию).

---

## 👥 Автор

**provemet** — разработчик архитектуры и реализации (v2.2, 2025-12-18)

---

## 📞 Поддержка

Для вопросов, ошибок и предложений создавайте issues в репозитории проекта.

---

**Версия документации:** 2.2  
**Дата обновления:** 2025-12-26  
**Статус:** ✅ Актуально
