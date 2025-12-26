#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>      // Для fopen, fprintf, fscanf
#include <sys/stat.h>   // Для mkdir
#include <errno.h>      // Для EEXIST
#include "s21_tetris_internals.h"

/* ---------------------- Вспомогательные макросы ---------------------- */

/**
 * @internal
 * @def TETRIS_INITIAL_SPEED
 * @brief Базовая скорость падения фигуры на уровне 1
 *
 * Задаёт начальное значение скорости (в условных единицах таймера), используемое
 * в начале игры. Скорость на последующих уровнях рассчитывается по формуле:
 * <code>current_speed = TETRIS_INITIAL_SPEED + level - 1</code>.
 *
 * Чем меньше значение, тем быстрее фигуры падают. Значение 1 — максимальная
 * начальная скорость (классический режим), значение 2 и выше — более медленное
 * падение, подходящее для новичков.
 *
 * @details
 * - Используется в функции инициализации состояния INIT (`on_enter_init`).
 * - Фактически определяет частоту тиков `TETRIS_EVT_TICK`, управляющих падением.
 * - Для плавного увеличения сложности рекомендуется, чтобы рост скорости
 *   оставался предсказуемым и монотонным.
 *
 * @note
 * - Должно быть положительным целым числом (≥ 1).
 * - Значение 1 — стандарт для большинства официальных реализаций Тетриса.
 * - Увеличение значения делает игру проще, уменьшение — сложнее.
 *
 * @warning Изменение значения может нарушить баланс игры, особенно если
 *          используется в сочетании с жёсткими ограничениями по времени.
 *          Тестируйте геймплей после модификации.
 *
 * @see on_enter_init(), tetris_apply_cleared_lines(), tetris_update_speed()
 */
#define TETRIS_INITIAL_SPEED 1

/**
 * @internal
 * @def TETRIS_TRANSITIONS_COUNT
 * @brief Количество переходов в таблице FSM игры Тетрис
 *
 * Вычисляет число элементов в массиве `tetris_transitions`, используемом
 * как таблица переходов конечного автомата. Применяется при инициализации FSM
 * для передачи размера таблицы в функцию `fsm_init()`.
 *
 * @details
 * Расчёт основан на стандартной идиоме C:
 * @code
 * #define TETRIS_TRANSITIONS_COUNT (sizeof(tetris_transitions) / sizeof((tetris_transitions)[0]))
 * @endcode
 * - Гарантирует вычисление на этапе компиляции.
 * - Работает только с массивами, известными на этапе компиляции.
 *
 * @note
 * - Макрос корректно работает **только с самим массивом `tetris_transitions`**.
 * - Не используйте его с указателями — результат будет некорректным
 *   (размер указателя / размер элемента).
 * - Является compile-time константой, пригодной для использования в статических утверждениях.
 *
 * @warning Несанкционированное использование с другими данными приведёт к логическим ошибкам.
 *          Не переиспользуйте этот макрос в других контекстах без проверки типа.
 *
 * @see tetris_transitions, fsm_init(), FSM_ASSERT_TRANSITION_TABLE()
 */
#define TETRIS_TRANSITIONS_COUNT \
    (sizeof(tetris_transitions) / sizeof(tetris_transitions[0]))

/* ----------------------- Данные переходов FSM ------------------------ */

// Forward callback declarations

static void on_enter_init(fsm_context_t ctx);
static void on_enter_spawn(fsm_context_t ctx);
static void on_enter_game_over(fsm_context_t ctx);
static void on_enter_paused(fsm_context_t ctx);
static void on_enter_lock(fsm_context_t ctx);
static void on_exit_paused(fsm_context_t ctx);

/**
 * @var tetris_transitions
 * @internal
 * @brief Таблица переходов конечного автомата (FSM) игры Тетрис
 *
 * Массив правил, определяющих поведение FSM: при каком текущем состоянии и событии
 * происходит переход в новое состояние, а также какие вызываются колбэки.
 *
 * Каждое правило типа `fsm_transition_t` задаёт:
 * - `from_state`: текущее состояние,
 * - `event`: входящее событие,
 * - `to_state`: состояние назначения,
 * - `on_enter`: функция, вызываемая при входе в новое состояние (или NULL),
 * - `on_exit`: функция, вызываемая при выходе из текущего (или NULL).
 *
 * @details
 * Ключевые переходы:
 * - INIT → SPAWN: при TETRIS_EVT_START — инициализация новой игры.
 * - SPAWN → FALL: при TETRIS_EVT_TICK — запуск падения новой фигуры.
 * - SPAWN → GAME_OVER: при TETRIS_EVT_NONE (условный автопереход) — если новая фигура
 *   не может быть размещена (поле переполнено).
 * - FALL → LOCK: при TETRIS_EVT_TICK — фиксация фигуры после неудачи в движении вниз.
 * - FALL → FALL: при MOVE/ROTATE — события обрабатываются внутри состояния, перехода нет.
 * - FALL → PAUSED: при TETRIS_EVT_PAUSE_TOGGLE — переход в паузу.
 * - FALL → GAME_OVER: при TETRIS_EVT_TERMINATE — принудительное завершение.
 * - LOCK → SPAWN: при TETRIS_EVT_TICK — после фиксации и обработки линий.
 * - PAUSED → FALL: при TETRIS_EVT_PAUSE_TOGGLE — возобновление игры.
 * - GAME_OVER → INIT: при TETRIS_EVT_START — перезапуск.
 *
 * @note
 * - Таблица использует внешнюю структуру `fsm_transition_t` из общего FSM-модуля.
 * - Колбэки (`on_enter_init`, `on_enter_spawn`, `on_enter_lock` и др.) определены
 *   в том же модуле и управляют инициализацией, спавном, обновлением скорости и т.д.
 * - FSM проверяет таблицу последовательно — первый подходящий переход используется.
 *   Порядок элементов **имеет значение**.
 *
 * @warning
 * - Нарушение порядка (например, более общие правила перед частными) может
 *   привести к пропуску переходов.
 * - Изменение логики (например, добавление/удаление состояний или событий) требует
 *   одновременного обновления TETRIS_TRANSITIONS_COUNT.
 * - Неправильные колбэки или состояния — источник зависаний, утечек или крахов.
 *
 * @see TetrisState, TetrisEvent, fsm_t, tetris_fsm_dispatch, fsm_transition_t, TETRIS_TRANSITIONS_COUNT
 */
static const fsm_transition_t tetris_transitions[] = {
    /* INIT → SPAWN: старт игры, спавним первую фигуру */
    { TETRIS_STATE_INIT,        TETRIS_EVT_START,     TETRIS_STATE_SPAWN,     NULL,           on_enter_spawn },

    /* SPAWN → FALL: начинаем падение. Фигура уже заспавнена — колбэка нет */
    { TETRIS_STATE_SPAWN,       TETRIS_EVT_TICK,      TETRIS_STATE_FALL,      NULL,           NULL },

    /* SPAWN → GAME_OVER: не удалось заспавнить */
    { TETRIS_STATE_SPAWN,       TETRIS_EVT_NONE,       TETRIS_STATE_GAME_OVER, NULL,           on_enter_game_over },

    /* FALL → LOCK: фигура уперлась, фиксируем */
    { TETRIS_STATE_FALL,        TETRIS_EVT_TICK,      TETRIS_STATE_LOCK,      NULL,           on_enter_lock },

    /* FALL → LOCK: движение вниз и жёсткий дроп */
    { TETRIS_STATE_FALL,        TETRIS_EVT_MOVE_DOWN, TETRIS_STATE_LOCK,      NULL,           on_enter_lock },
    { TETRIS_STATE_FALL,        TETRIS_EVT_DROP,      TETRIS_STATE_LOCK,      NULL,           on_enter_lock },

    /* Движения и повороты в FALL — без переходов */
    { TETRIS_STATE_FALL,        TETRIS_EVT_MOVE_LEFT,   TETRIS_STATE_FALL,    NULL,           NULL },
    { TETRIS_STATE_FALL,        TETRIS_EVT_MOVE_RIGHT,  TETRIS_STATE_FALL,    NULL,           NULL },
    { TETRIS_STATE_FALL,        TETRIS_EVT_ROTATE,      TETRIS_STATE_FALL,    NULL,           NULL },

    /* Пауза */
    { TETRIS_STATE_FALL,        TETRIS_EVT_PAUSE_TOGGLE, TETRIS_STATE_PAUSED, NULL,           on_enter_paused },
    { TETRIS_STATE_PAUSED,      TETRIS_EVT_PAUSE_TOGGLE, TETRIS_STATE_FALL,   on_exit_paused, NULL },

    /* TERMINATE — в любой момент */
    { TETRIS_STATE_FALL,        TETRIS_EVT_TERMINATE, TETRIS_STATE_GAME_OVER, NULL,           on_enter_game_over },

    /* LOCK → SPAWN: после фиксации — новая фигура */
    { TETRIS_STATE_LOCK,        TETRIS_EVT_TICK,      TETRIS_STATE_SPAWN,     NULL,           on_enter_spawn },

    /* GAME_OVER → INIT: перезапуск */
    { TETRIS_STATE_GAME_OVER,   TETRIS_EVT_NONE,     TETRIS_STATE_INIT,      NULL,           on_enter_init },
};

/* --------------------------- Утилиты фигур --------------------------- */

const int (*tetris_getPieceData(int type, int rotation))[4] {
    if (type < 0 || type >= TETRIS_NUM_PIECES) return NULL;
    if (rotation < 0 || rotation >= TETRIS_ROTATIONS) return NULL;

    return TETROMINO_DATA[type][rotation];
}

/* ---------------------- Внутренние утилиты поля ---------------------- */

/**
 * @internal
 * @brief Выделяет и инициализирует двумерную матрицу в динамической памяти
 *
 * Создаёт матрицу размером rows × cols, где каждый элемент инициализирован нулём.
 * Алгоритм:
 * - Выделяет массив указателей на строки (int**) размером `rows`.
 * - Для каждой строки выделяет блок памяти (int*) размером `cols`.
 * - При любом сбое освобождает уже выделенные ресурсы и возвращает NULL.
 *
 * @param[in] rows Количество строк. Должно быть строго больше 0.
 * @param[in] cols Количество столбцов. Должно быть строго больше 0.
 * @return Указатель на выделенную матрицу (int**) при успехе, NULL при ошибке.
 *
 * @details
 * - Использует `calloc()` для строк и элементов — гарантируется обнуление памяти.
 * - Реализует exception-safe освобождение: при неудаче все промежуточные блоки
 *   освобождаются, утечек не происходит.
 * - Подходит для создания игрового поля и буферов фигур.
 *
 * @note
 * - Корректно обрабатывает частичный отказ памяти (например, нехватку на 5-й строке).
 * - Требуемая память: sizeof(int*) * rows + sizeof(int) * rows * cols.
 * - Результат должен быть освобождён через tetris_free_matrix() для избежания утечек.
 *
 * @warning
 * - Поведение не определено при rows ≤ 0 или cols ≤ 0.
 * - Не проверяется переполнение при умножении `rows * cols`.
 *   Убедитесь, что размеры находятся в допустимых пределах.
 *
 * @see tetris_free_matrix(), tetris_game_create(), TETRIS_FIELD_ROWS, TETRIS_FIELD_COLS
 */
static int **tetris_alloc_matrix(int rows, int cols) {
    int **m = calloc(rows, sizeof(int *));
    if (!m) return NULL;
    for (int r = 0; r < rows; ++r) {
        m[r] = calloc(cols, sizeof(int));
        if (!m[r]) {
            for (int i = 0; i < r; ++i) free(m[i]);
            free(m);
            return NULL;
        }
    }
    return m;
}

/**
 * @internal
 * @brief Освобождает двумерную матрицу, выделенную через tetris_alloc_matrix
 *
 * Деаллоцирует память, занятую динамически выделенной матрицей:
 * - сначала освобождает каждую строку,
 * - затем освобождает массив указателей на строки.
 * Корректно обрабатывает NULL-указатель — в этом случае функция не выполняет действий.
 *
 * @param[in] m     Указатель на матрицу (массив указателей на int). Может быть NULL.
 * @param[in] rows  Количество строк в матрице. Должно соответствовать количеству,
 *                  переданному при выделении через tetris_alloc_matrix.
 *
 * @details
 * - Если `m` равен NULL, функция завершается немедленно — поведение безопасно.
 * - Для каждой строки `m[r]` вызывается `free(m[r])`.
 * - После освобождения всех строк вызывается `free(m)` для массива указателей.
 * - Функция не устанавливает `m` в NULL — это остаётся ответственностью вызывающего кода.
 *
 * @note
 * - Должна вызываться только для матриц, выделенных через tetris_alloc_matrix().
 * - Используется в tetris_game_destroy() для очистки field_storage и next_storage.
 * - Параметр `rows` должен быть точным — его несоответствие приведёт к неопределённому поведению.
 *
 * @warning
 * - Повторный вызов с теми же параметрами (double free) вызывает неопределённое поведение.
 * - Не является потокобезопасной — требует внешней синхронизации при использовании из нескольких потоков.
 * - Не проверяет, был ли блок уже освобождён.
 *
 * @see tetris_alloc_matrix(), tetris_game_destroy()
 */
static void tetris_free_matrix(int **m, int rows) {
    if (!m) return;
    for (int r = 0; r < rows; ++r) {
        free(m[r]);
    }
    free(m);
}

/* ----------------------- Жизненный цикл игры ------------------------ */

TetrisGame *tetris_game_create(void) {
    TetrisGame *game = calloc(1, sizeof(TetrisGame));
    if (!game) return NULL;

    game->field_storage = tetris_alloc_matrix(TETRIS_FIELD_ROWS, TETRIS_FIELD_COLS);
    game->next_storage  = tetris_alloc_matrix(TETRIS_NEXT_SIZE, TETRIS_NEXT_SIZE);
    if (!game->field_storage || !game->next_storage) {
        tetris_game_destroy(game);
        return NULL;
    }

    game->info.next  = game->next_storage;

    // Инициализируем указатели в info
    game->info.field = tetris_alloc_matrix(TETRIS_FIELD_ROWS, TETRIS_FIELD_COLS);
    if (!game->info.field) {
        tetris_game_destroy(game);
        return NULL;
    }

    tetris_load_high_score(game);

    tetris_game_reset(game);

    return game;
}

void tetris_game_destroy(TetrisGame *game) {
    if (!game) return;

    tetris_save_high_score(game);
    tetris_free_matrix(game->field_storage, TETRIS_FIELD_ROWS);
    tetris_free_matrix(game->next_storage,  TETRIS_NEXT_SIZE);
    tetris_free_matrix(game->info.field, TETRIS_FIELD_ROWS);
    free(game);
}

void tetris_game_reset(TetrisGame *game) {
    if (!game) return;
    
    // Вызываем on_enter_init напрямую, так как FSM не гарантирует его вызов
    on_enter_init((fsm_context_t)game);

    fsm_init(&game->fsm, (fsm_context_t)game,
             tetris_transitions, TETRIS_TRANSITIONS_COUNT,
             TETRIS_STATE_INIT);
}
/* -------------------------- Поле и превью ---------------------------- */

void tetris_clear_field(TetrisGame *game) {
    if (!game || !game->field_storage || !game->info.field) return;
    for (int r = 0; r < TETRIS_FIELD_ROWS; ++r) {
        for (int c = 0; c < TETRIS_FIELD_COLS; ++c) {
            game->field_storage[r][c] = 0;
            game->info.field[r][c] = 0;
        }
    }
}

void tetris_clear_next(TetrisGame *game) {
    if (!game || !game->next_storage) return;
    for (int r = 0; r < TETRIS_NEXT_SIZE; ++r) {
        for (int c = 0; c < TETRIS_NEXT_SIZE; ++c) {
            game->next_storage[r][c] = 0;
        }
    }
}

void tetris_update_next_preview(TetrisGame *game) {
    if (!game) return;

    tetris_clear_next(game);

    const int (*shape)[4] = tetris_getPieceData(game->next.type,
                                                game->next.rotation);
    if (!shape) return;

    for (int r = 0; r < 4 && r < TETRIS_NEXT_SIZE; ++r) {
        for (int c = 0; c < 4 && c < TETRIS_NEXT_SIZE; ++c) {
            if (shape[r][c]) {
                game->next_storage[r][c] = shape[r][c];
            }
        }
    }
}

/* --------------- Коллизии, фиксация и линии (заглушки) --------------- */

bool tetris_check_collision(const TetrisGame *game,
                            const TetrisPiece *piece) {
    if (!game || !piece) return true;

    const int (*shape)[4] = tetris_getPieceData(piece->type,
                                                piece->rotation);
    if (!shape) return true;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (!shape[row][col]) continue;

            int field_row = piece->y + row;
            int field_col = piece->x + col;

            // Проверка левой/правой границы
            if (field_col < 0 || field_col >= TETRIS_FIELD_COLS) {
                return true;
            }

            // Пропускаем клетки выше верхней границы (спавн)
            if (field_row < 0) {
                continue;
            }

            // Проверка нижней границы (пол)
            if (field_row >= TETRIS_FIELD_ROWS) {
                return true;
            }

            // Проверка пересечения с полем
            if (game->field_storage[field_row][field_col] != 0) {
                return true;
            }
        }
    }

    return false;
}

void tetris_lock_piece(TetrisGame *game) {
    if (!game) return;

    const int (*shape)[4] = tetris_getPieceData(game->current.type,
                                                game->current.rotation);
    if (!shape) return;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (!shape[row][col]) continue;

            int field_row = game->current.y + row;
            int field_col = game->current.x + col;

            if (field_row < 0 || field_row >= TETRIS_FIELD_ROWS ||
                field_col < 0 || field_col >= TETRIS_FIELD_COLS) {
                continue;
            }

            game->field_storage[field_row][field_col] = shape[row][col];
        }
    }
}

/**
 * @brief Проверяет, полностью ли заполнена указанная строка на игровом поле
 *
 * Проверяет, что все ячейки в строке с индексом `row` содержат ненулевые значения
 * (то есть блоки), что означает её полное заполнение. Используется как вспомогательная
 * функция при анализе поля на предмет необходимости удаления строк.
 *
 * @param[in] game Указатель на константный экземпляр TetrisGame. Может быть NULL.
 * @param[in] row  Индекс строки для проверки (0 — верхняя строка).
 *                 Допустимые значения: от 0 до TETRIS_FIELD_ROWS - 1.
 *
 * @return true, если строка существует и все её ячейки заполнены (не равны 0);
 *         false, если:
 *         - game == NULL,
 *         - row выходит за границы поля,
 *         - хотя бы одна ячейка в строке равна 0.
 *
 * @details
 * Алгоритм:
 * - Если game == NULL — возвращается false.
 * - Если row < 0 или row >= TETRIS_FIELD_ROWS — возвращается false.
 * - Итерация по всем столбцам строки (от 0 до TETRIS_FIELD_COLS - 1):
 *   - если любая ячейка game->field_storage[row][c] == 0 — возвращается false.
 * - Если все ячейки ненулевые — возвращается true.
 *
 * Функция не учитывает тип блока — важно только, что ячейка занята.
 *
 * @note
 * - Является чистой (pure) функцией: зависит только от входных данных и не изменяет состояние.
 * - Используется исключительно внутри tetris_clear_full_lines() для определения строк к удалению.
 * - Линейная сложность O(cols) — оптимальна для однократной проверки строки.
 *
 * @warning
 * - Не выполняет отладочную печать — ошибка проявляется только через возвращаемое значение.
 * - Не является потокобезопасной, если game->field_storage может изменяться из другого потока.
 *
 * @see tetris_clear_full_lines()
 * @internal
 */
static bool tetris_is_line_full(const TetrisGame *game, int row) {
    if (!game || row < 0 || row >= TETRIS_FIELD_ROWS) {
        return false;
    }

    for (int col = 0; col < TETRIS_FIELD_COLS; ++col) {
        if (game->field_storage[row][col] == 0) {
            return false;
        }
    }

    return true;
}

int tetris_clear_full_lines(TetrisGame *game) {
    if (!game) return 0;
    
    // Шаг 1: Собрать все полные строки (их индексы)
    int full_lines[TETRIS_FIELD_ROWS];
    int full_count = 0;
    
    for (int row = 0; row < TETRIS_FIELD_ROWS; ++row) {
        if (tetris_is_line_full(game, row)) {
            full_lines[full_count++] = row;
        }
    }
    
    if (full_count == 0) return 0;
    
    // Шаг 2: Один раз скопировать весь блок вниз
    int first_full = full_lines[0];
    int rows_to_move = TETRIS_FIELD_ROWS - first_full - full_count;
    if (rows_to_move > 0) {
        memmove(
            &game->field_storage[first_full][0],
            &game->field_storage[first_full + full_count][0],
            rows_to_move * TETRIS_FIELD_COLS * sizeof(int) );
    }
    
    // Шаг 3: Очистить top full_count строк
    for (int row = TETRIS_FIELD_ROWS - full_count; row < TETRIS_FIELD_ROWS; ++row) {
        memset(game->field_storage[row], 0, TETRIS_FIELD_COLS * sizeof(int));
    }
    
    return full_count;
}
/* ------------------ Спавн и движение фигуры ---------------- */

void tetris_generate_next_piece(TetrisGame *game) {
    if (!game) return;
    game->next.type = rand() % TETRIS_NUM_PIECES;
    game->next.rotation = rand() % TETRIS_ROTATIONS;
    game->next.x = TETRIS_FIELD_COLS / 2 - 2;
    game->next.y = 0;
}

void tetris_spawn_piece(TetrisGame *game) {
    if (!game) return;

    game->current = game->next;
    tetris_generate_next_piece(game);
    tetris_update_next_preview(game);
    if (tetris_check_collision(game, &game->current)) {
        game->game_over = true;
    }
}

bool tetris_move_piece(TetrisGame *game, int dx, int dy) {
    if (!game) return false;

    TetrisPiece tmp = game->current;
    tmp.x += dx;
    tmp.y += dy;
    if (tetris_check_collision(game, &tmp)) {
        return false;
    }
    game->current = tmp;

    return true;
}

bool tetris_rotate_piece(TetrisGame *game, int direction) {
    if (!game) return false;

    TetrisPiece tmp = game->current;
    tmp.rotation = (tmp.rotation + direction) % TETRIS_ROTATIONS;
    if (tmp.rotation < 0) tmp.rotation += TETRIS_ROTATIONS;
    if (tetris_check_collision(game, &tmp)) {
        return false;
    }
    game->current = tmp;

    return true;
}

/* ---------------- Счёт, уровень, рекорд -------------------- */

void tetris_apply_cleared_lines(TetrisGame *game, int lines) {
    if (!game || lines <= 0 || lines > 4 ) return;

    game->lines_cleared += lines;
    int score_bonus;
    switch (lines) {
        case 1: score_bonus = 100;   break;   // Очки за одну линию
        case 2: score_bonus = 300;   break;   // две линии  
        case 3: score_bonus = 700;   break;   // три
        case 4: score_bonus = 1500;  break;   // Четыре!
        default: score_bonus = 0;
    }
    game->info.score +=  score_bonus;
    game->info.level = 1 + game->lines_cleared / 10;
    game->info.speed = TETRIS_INITIAL_SPEED + game->info.level - 1;
    tetris_update_high_score(game);
}

void tetris_update_high_score(TetrisGame *game) {
    if (!game) return;
    
    if (game->info.score > game->high_score) {
        game->high_score = game->info.score;
        game->info.high_score = game->info.score;
    } else {
        game->info.high_score = game->high_score; 
    }
}

bool tetris_load_high_score(TetrisGame *game) {
    if (!game) return false;
    
    // Открываем файл для чтения
    FILE *file = fopen(TETRIS_SCORE_FILE, "r");
    if (!file) {
        game->high_score = 0;
        game->info.high_score = 0;
        return true;
    }
    
    // Читаем рекорд из файла
    int score = -1;
    int result = fscanf(file, "%d", &score);
    if (result != 1) {
        fclose(file);
        return false;
    }
    
    // Валидируем и устанавливаем рекорд
    if (score >= 0) {
        game->high_score = score;
        game->info.high_score = score;
        fclose(file);
        return true;
    }
    
    // Некорректное значение рекорда
    game->high_score = 0;
    game->info.high_score = 0;

    fclose(file);

    return false;
}

bool tetris_save_high_score(const TetrisGame *game) {
    if (!game) return false;
    
    // Создаём директорию .brickgame если её нет
    int mkdir_result = mkdir(TETRIS_SCORE_DIR, 0755);
    if (mkdir_result != 0 && errno != EEXIST) {
        return false;  // Ошибка создания директории (и это не "директория уже существует")
    }
    
    // Открываем файл для записи
    FILE *file = fopen(TETRIS_SCORE_FILE, "w");
    if (!file) return false;
    
    // Записываем рекорд в файл (одно число)
    int written = fprintf(file, "%d\n", game->high_score);
    fclose(file);
    
    // Проверяем, что что-то было написано
    return written > 0;
}

/* --------------------------- FSM Tetris -------------------------------- */

/**
 * @brief Колбэк, вызываемый при входе в начальное состояние инициализации
 *
 * Полностью сбрасывает состояние игры к начальному: обнуляет счёт, уровень, флаги,
 * игровое поле и превью. Восстанавливает корректные ссылки в GameInfo_t и подготавливает
 * экземпляр TetrisGame к началу новой партии.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * Выполняемые действия:
 * - Приведение ctx к TetrisGame*; проверка на NULL.
 * - Обнуление всех полей game->info через memset, кроме указателей на буферы.
 * - Восстановление ссылок:
 *   - info.field = game->field_storage
 *   - info.next  = game->next_storage
 * - Сброс игровых метрик:
 *   - score = 0, level = 1, speed = TETRIS_INITIAL_SPEED
 *   - lines_cleared = 0
 *   - game_over = false, pause = false
 * - Синхронизация info.high_score с текущим значением game->high_score (из памяти)
 * - Очистка игрового поля (tetris_clear_field) и буфера превью (tetris_clear_next)
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Вызывается при переходе в TETRIS_STATE_INIT как при старте игры, так и при перезапуске.
 * - Гарантирует чистое состояние, но **не запускает логику игры** — спавн фигуры происходит в TETRIS_STATE_SPAWN.
 *
 * @warning
 * - **Не загружает рекорд с диска** — ожидается, что game->high_score уже установлен
 *   (например, через tetris_load_high_score() перед инициализацией FSM).
 * - Не проверяет, инициализированы ли буферы field_storage/next_storage — предполагается,
 *   что память выделена на более высоком уровне (tetris_game_create).
 * - Не является потокобезопасной: требует эксклюзивный доступ к game.
 *
 * @see tetris_fsm_dispatch(), tetris_game_create(), tetris_load_high_score(),
 *      tetris_clear_field(), tetris_clear_next()
 * @internal
 */
static void on_enter_init(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;
    
    //memset(&game->info, 0, sizeof(GameInfo_t));
    game->lines_cleared = 0;
    game->game_over = false;
    game->started = true;
    game->info.score = 0;
    game->info.level = 1;
    game->info.speed = TETRIS_INITIAL_SPEED;
    game->info.pause = 0;
    game->info.high_score = game->high_score;
    game->current.type = 0;
    game->current.rotation = 0;
    game->current.x = 0;
    game->current.y = 0;
    
    tetris_clear_field(game);
    tetris_clear_next(game);

    tetris_generate_next_piece(game);
    tetris_update_next_preview(game);
}

/**
 * @brief Колбэк, вызываемый при входе в состояние спавна новой фигуры
 *
 * Инициирует появление новой активной фигуры: перемещает фигуру из `next` в `current`,
 * генерирует новую следующую фигуру, обновляет визуальное превью и проверяет,
 * может ли фигура быть размещена в начальной позиции. Если размещение невозможно
 * (например, из-за заполнения верхних строк), устанавливается флаг `game_over`.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * Последовательность действий:
 * 1. Приведение ctx к TetrisGame*; проверка на NULL.
 * 2. Вызов tetris_spawn_piece(game), который:
 *    - Копирует game->next в game->current,
 *    - Генерирует новую следующую фигуру,
 *    - Обновляет буфер превью,
 *    - Проверяет коллизию и при необходимости устанавливает game->game_over = true.
 *
 * Функция не выполняет дополнительных проверок — полагается на логику tetris_spawn_piece().
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Вызывается как при старте игры (после TETRIS_STATE_INIT), так и после фиксации фигуры.
 * - После успешного спавна игра ожидает переход в TETRIS_STATE_FALL для управления падением.
 *
 * @warning
 * - **Не проверяет, была ли предыдущая фигура корректно зафиксирована** —
 *   это ответственность FSM и предыдущих состояний (например, TETRIS_STATE_LOCK).
 * - Не очищает поле и не сбрасывает счёт — предполагается, что игра находится в валидном состоянии.
 * - Не является потокобезопасной: требует эксклюзивный доступ к игровому состоянию.
 *
 * @see tetris_fsm_dispatch(), tetris_spawn_piece(), tetris_state_init_enter(), tetris_state_lock_enter()
 * @internal
 */
static void on_enter_spawn(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;

    game->info.pause = 0;
    tetris_spawn_piece(game);
}

/**
 * @brief Колбэк, вызываемый при входе в состояние фиксации фигуры (LOCK)
 *
 * Выполняет цепочку действий при завершении движения активной фигуры:
 * 1. Фиксирует текущую фигуру на игровом поле,
 * 2. Удаляет все полностью заполненные строки,
 * 3. Начисляет очки, обновляет уровень и скорость в зависимости от количества линий.
 *
 * Используется конечным автоматом при переходе в состояние TETRIS_STATE_LOCK.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * Последовательность операций:
 * 1. Приведение ctx к TetrisGame*; проверка на NULL.
 * 2. Фиксация фигуры: вызов tetris_lock_piece(game).
 * 3. Проверка и удаление полных строк: tetris_clear_full_lines(game).
 *    - Возвращает количество удалённых строк (0–4).
 * 4. Если строки были удалены — вызов tetris_apply_cleared_lines(game, lines),
 *    который обновляет счёт, уровень, скорость и синхронизирует рекорд.
 *
 * Этот колбэк гарантирует, что все изменения состояния игры (счёт, поле, уровень)
 * происходят атомарно в один момент — при фиксации фигуры.
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Вызывается автоматически, когда фигура не может двигаться вниз (после TETRIS_STATE_FALL).
 * - После завершения FSM переходит в TETRIS_STATE_SPAWN для появления новой фигуры.
 * - Даже если `tetris_lock_piece` не зафиксировал ни одной ячейки (например, фигура частично вне поля),
 *   попытка удаления строк и начисления очков всё равно выполняется.
 *
 * @warning
 * - Не проверяет корректность текущей фигуры — предполагается, что она валидна.
 * - Не изменяет `game->current` — очистка и смена фигуры происходят в `tetris_state_spawn_enter`.
 * - Не является потокобезопасной: требует эксклюзивный доступ к игровому состоянию.
 *
 * @see tetris_fsm_dispatch(), tetris_lock_piece(), tetris_clear_full_lines(), tetris_apply_cleared_lines(),
 *      tetris_state_spawn_enter()
 * @internal
 */
static void on_enter_lock(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;
    
    tetris_lock_piece(game);
    int cleared = tetris_clear_full_lines(game);
    
    if (cleared > 0) {
        tetris_apply_cleared_lines(game, cleared);
    }
}

/**
 * @brief Колбэк, вызываемый при входе в состояние паузы
 *
 * Устанавливает флаг паузы в публичной структуре состояния игры (info.pause = true),
 * сигнализируя интерфейсу и внешним системам о том, что игра приостановлена.
 * Используется конечным автоматом при переходе в состояние TETRIS_STATE_PAUSED.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * - Приводит ctx к TetrisGame* и проверяет на NULL.
 * - Устанавливает game->info.pause = true, чтобы UI мог отобразить состояние паузы.
 * - Не изменяет игровое поле, текущую фигуру, счёт, уровень, скорость или флаги игры.
 * - Все игровые данные остаются неизменными — игра может быть возобновлена в любой момент.
 *
 * Функция входит в FSM-архитектуру и вызывается автоматически при событии TETRIS_EVT_PAUSE_TOGGLE,
 * когда текущее состояние — TETRIS_STATE_FALL.
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Состояние паузы не влияет на логику таймера — ожидается, что внешняя система
 *   (например, таймер в main loop) проверяет info.pause перед обработкой тиков.
 * - После возобновления игра продолжается с той же фигуры и того же места.
 *
 * @warning
 * - Не проверяет корректность game->info — предполагается, что структура инициализирована.
 * - Не является потокобезопасной: изменение info.pause должно быть синхронизировано,
 *   если доступ к info осуществляется из другого потока.
 * - Не блокирует ввод — события могут продолжать поступать, но игнорируются в состоянии PAUSED.
 *
 * @see tetris_fsm_dispatch(), tetris_state_paused_exit(), TetrisState, fsm_process_event()
 * @internal
 */
static void on_enter_paused(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;
    
    game->info.pause = 1;
}

/**
 * @brief Колбэк, вызываемый при выходе из состояния паузы в игре Тетрис
 *
 * Выполняется при покидании состояния TETRIS_STATE_PAUSED и возобновлении игры.
 * Сбрасывает флаг паузы в публичной структуре состояния (info.pause = false),
 * сигнализируя интерфейсу и внешним системам о том, что игра снова активна.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * - Приводит ctx к TetrisGame* и проверяет на NULL.
 * - Устанавливает game->info.pause = false, чтобы UI мог обновить отображение состояния.
 * - Не изменяет игровое поле, текущую фигуру, счёт, уровень, скорость или флаги игры.
 * - Все параметры сохраняются — игра продолжается с того же места.
 *
 * Колбэк вызывается автоматически функцией `fsm_process_event()` при переходе
 * из состояния TETRIS_STATE_PAUSED в TETRIS_STATE_FALL по событию TETRIS_EVT_PAUSE_TOGGLE.
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Используется исключительно в таблице переходов состояний.
 * - Ожидается, что внешний цикл (например, main loop) проверяет `info.pause`
 *   перед обработкой игровых тиков (TETRIS_EVT_TICK).
 *
 * @warning
 * - Не проверяет корректность game->info — предполагается, что структура инициализирована.
 * - Не является потокобезопасной: изменение `info.pause` должно быть синхронизировано,
 *   если доступ к `info` осуществляется из другого потока.
 * - Передача невалидного указателя (не NULL, но указывающего на мусор) приводит к неопределённому поведению.
 *
 * @see tetris_fsm_dispatch(), tetris_state_paused_enter(), TetrisState, fsm_process_event()
 * @internal
 */
static void on_exit_paused(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;
    
    game->info.pause = 0;
}

/**
 * @brief Колбэк, вызываемый при входе в состояние "Игра окончена"
 *
 * Устанавливает флаг окончания игры в `true`, сигнализируя всем внешним системам
 * (например, пользовательскому интерфейсу), что текущая партия завершена.
 * Используется конечным автоматом при переходе в состояние TETRIS_STATE_GAME_OVER.
 *
 * @param[in] ctx Указатель на контекст состояния — должен быть приведён к TetrisGame*.
 *                Может быть NULL — в этом случае функция безопасно завершается.
 *
 * @details
 * - Приводит `ctx` к `TetrisGame*` и проверяет на `NULL`.
 * - Устанавливает `game->game_over = true`, что используется для:
 *   - блокировки ввода,
 *   - отображения экрана "Game Over",
 *   - остановки игрового таймера внешним циклом.
 * - Не изменяет игровое поле, счёт, уровень, фигуры или рекорд — все данные остаются доступными.
 * - Не инициирует сохранение рекорда — ожидается, что `tetris_save_high_score()`
 *   будет вызвана до или после этого перехода.
 *
 * Колбэк вызывается автоматически при переходе в состояние GAME_OVER,
 * инициированном, например, неудачным спавном фигуры.
 *
 * @note
 * - Является внутренним колбэком FSM — **не предназначена для прямого вызова**.
 * - Используется исключительно в таблице переходов состояний.
 * - После активации состояния игра не обрабатывает движение, поворот или тики,
 *   пока не будет перезапущена (через переход в INIT или SPAWN).
 *
 * @warning
 * - Не проверяет корректность `game->info` — предполагается, что структура инициализирована.
 * - Не является потокобезопасной: изменение `game_over` должно быть синхронизировано,
 *   если доступ к состоянию осуществляется из другого потока.
 * - Передача невалидного указателя (не NULL, но указывающего на мусор) приводит к неопределённому поведению.
 *
 * @see tetris_fsm_dispatch(), tetris_state_init_enter(), tetris_spawn_piece(),
 *      tetris_save_high_score(), TetrisState
 * @internal
 */
static void on_enter_game_over(fsm_context_t ctx) {
    TetrisGame *game = (TetrisGame *)ctx;
    if (!game) return;
    
    game->game_over = true;
}

/**
 * @brief Обрабатывает пользовательский ввод и тики при активном падении фигуры
 *
 * Обрабатывает события в состоянии TETRIS_STATE_FALL: перемещение, поворот,
 * мягкое (MOVE_DOWN) и жёсткое падение (DROP). Преобразует события для корректной
 * передачи в конечный автомат: подавляет обработанные действия и возвращает
 * специальные команды для переходов при блокировке падения.
 *
 * @param[in] game  Указатель на экземпляр TetrisGame. Может быть NULL — в этом случае
 *                  возвращается исходное событие без изменений.
 * @param[in] event Входящее событие (например, TETRIS_EVT_MOVE_LEFT, TETRIS_EVT_TICK и др.)
 *
 * @return Преобразованное событие для FSM:
 *         - TETRIS_EVT_NONE — событие полностью обработано (например, движение выполнено),
 *         - TETRIS_EVT_TICK — фигура не может двигаться вниз; требуется переход в TETRIS_STATE_LOCK,
 *         - Исходное событие — если оно не обработано и должно быть обработано FSM (например, пауза).
 *
 * @details
 * Логика обработки:
 * - TETRIS_EVT_MOVE_LEFT / RIGHT: вызывается tetris_move_piece(), при успехе возвращается NONE.
 * - TETRIS_EVT_ROTATE: вызывается tetris_rotate_piece(), при успехе — NONE.
 * - TETRIS_EVT_MOVE_DOWN / TETRIS_EVT_TICK:
 *   - Пытается сдвинуть фигуру вниз через tetris_move_piece(game, 0, 1).
 *   - Если движение невозможно — возвращается TETRIS_EVT_TICK (инициирует LOCK).
 *   - Если возможно — возвращается NONE (состояние не меняется).
 * - TETRIS_EVT_DROP:
 *   - Выполняет hard drop: циклически вызывает tetris_move_piece(game, 0, 1), пока возможно.
 *   - После остановки возвращает TETRIS_EVT_TICK для немедленной фиксации.
 * - Все остальные события (например, TETRIS_EVT_PAUSE_TOGGLE) возвращаются без изменений,
 *   чтобы FSM мог корректно обработать переходы (например, в состояние PAUSED).
 *
 * @note
 * - Функция вынесена для упрощения tetris_fsm_dispatch() — делает её чище и понятнее.
 * - Является внутренней (static) — **не предназначена для вызова извне**.
 * - Гарантирует, что FSM получает только значимые для переходов события.
 *
 * @warning
 * - Не проверяет текущее состояние — **должна вызываться только в TETRIS_STATE_FALL**.
 * - Поведение не определено, если game->current содержит некорректные данные.
 * - Не является потокобезопасной: требует синхронизации при доступе к состоянию игры.
 *
 * @see tetris_fsm_dispatch(), tetris_move_piece(), tetris_rotate_piece(), TetrisEvent
 * @internal
 */
static TetrisEvent tetris_process_fall_input(TetrisGame *game, TetrisEvent event) {
    if (!game) return event;

    switch (event) {
        case TETRIS_EVT_MOVE_LEFT:
            tetris_move_piece(game, -1, 0);
            return TETRIS_EVT_NONE;
        case TETRIS_EVT_MOVE_RIGHT:
            tetris_move_piece(game, 1, 0);
            return TETRIS_EVT_NONE;
        case TETRIS_EVT_ROTATE:
            tetris_rotate_piece(game, 1);
            return TETRIS_EVT_NONE;
        default:
            break;
    }

    if (event == TETRIS_EVT_TICK || event == TETRIS_EVT_MOVE_DOWN) {
        bool can_move = tetris_move_piece(game, 0, 1);
        if (!can_move) {
            return TETRIS_EVT_TICK;  // → LOCK
        }
        return TETRIS_EVT_NONE;
    }

    if (event == TETRIS_EVT_DROP) {
        while (tetris_move_piece(game, 0, 1)) {
            /* падаем до упора */
        }
        return TETRIS_EVT_TICK;
    }

    return event;
}

void tetris_fsm_dispatch(TetrisGame *game, TetrisEvent event) {
    if (!game) return;

    // Состояние до обработки
    // fsm_state_t prev_state = game->fsm.current;

    // Обработка падения
    if ((TetrisState)game->fsm.current == TETRIS_STATE_FALL) {
        event = tetris_process_fall_input(game, event);

        if (event == TETRIS_EVT_NONE) {
            return;
        }
    }

    // Обычный FSM
    fsm_process_event(&game->fsm, event);

    // Автоматические переходы
    if ((TetrisState)game->fsm.current == TETRIS_STATE_SPAWN && game->game_over) {
        fsm_update(&game->fsm);
    }

}