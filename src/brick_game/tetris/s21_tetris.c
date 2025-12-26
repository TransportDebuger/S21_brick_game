/**
 * @file s21_tetris.c
 * @brief Реализация публичного API игры Тетрис
 *
 * Содержит функции-адаптеры, связывающие внешний интерфейс игры (s21_tetris.h)
 * с внутренней логикой, реализованной в s21_tetris_internals.c. Преобразует
 * пользовательские действия в события конечного автомата (FSM), управляет
 * жизненным циклом игры и обеспечивает интеграцию с общим движком игр
 * через шаблон GameInterface_t.
 *
 * @details Основные компоненты:
 * - tetris_map_action_to_event() — сопоставление действий пользователя с событиями FSM
 * - Обёртки для внутренних функций: create, destroy, input, update, get_info
 * - Реализация tetris_get_interface() для динамической регистрации в движке
 *
 * Архитектура следует паттерну "Фасад": скрывает сложность внутренней реализации,
 * обеспечивает чёткий, легко тестируемый и расширяемый интерфейс.
 *
 * @note Все функции являются потоконебезопасными и должны вызываться из одного потока.
 *       Внутренняя структура TetrisGame скрыта через неполную декларацию.
 *
 * @warning Некорректные указатели (NULL) обрабатываются устойчиво — функции безопасно
 *          завершаются без побочных эффектов. Не передавайте невалидные указатели.
 *
 * @author provemet
 * @version 2.2
 * @date 2025-12-18
 * @see s21_tetris.h, s21_tetris_internals.c, s21_tetris_internals.h
 */
#include "s21_tetris.h"
#include "s21_tetris_internals.h"

/* --------------------- Вспомогательные функции ---------------------- */

/**
 * @internal
 * @brief Преобразует пользовательское действие в событие конечного автомата игры Тетрис
 *
 * Функция сопоставляет входное действие пользователя (нажатие клавиши, удержание)
 * с внутренним событием FSM (TetrisEvent), которое будет обработано машиной состояний.
 * Учитывает флаг удержания клавиши (hold) для различия между мягким и жёстким падением.
 *
 * @param[in] action Тип действия пользователя, заданный через перечисление UserAction_t.
 *                   Возможные значения: Start, Left, Right, Down, Up, Action, Pause, Terminate.
 * @param[in] hold   Флаг, указывающий, удерживается ли клавиша (true — удержание, false — одиночное нажатие).
 *                   Используется для определения поведения при нажатии клавиши "вниз".
 *
 * @return Значение типа TetrisEvent, соответствующее семантике действия:
 *         - При action == Down и hold == true: TETRIS_EVT_DROP — мгновенное падение (hard drop).
 *         - При action == Down и hold == false: TETRIS_EVT_MOVE_DOWN — пошаговое движение вниз.
 *         - Остальные действия преобразуются в соответствующие события.
 *         - Неподдерживаемые или необрабатываемые действия (включая Up) возвращают TETRIS_EVT_NONE.
 *
 * @details
 * Примеры преобразований:
 * - Left → TETRIS_EVT_MOVE_LEFT
 * - Pause → TETRIS_EVT_PAUSE_TOGGLE
 * - Down + hold → TETRIS_EVT_DROP
 * - Up → TETRIS_EVT_NONE (не используется в логике)
 *
 * @note Функция является внутренней (static) и предназначена только для использования
 *       в рамках модуля обработки ввода Тетриса. Не экспортируется за пределы файла.
 *
 * @warning Событие TETRIS_EVT_NONE игнорируется конечным автоматом — такие действия
 *          не приводят к изменению состояния игры.
 *
 * @see UserAction_t, TetrisEvent, tetris_handle_input(), tetris_fsm_update()
 */
static TetrisEvent tetris_map_action_to_event(UserAction_t action, bool hold) {
    switch (action) {
        case Start:
            return TETRIS_EVT_START;
        case Left:
            return TETRIS_EVT_MOVE_LEFT;
        case Right:
            return TETRIS_EVT_MOVE_RIGHT;
        case Down:
            return hold ? TETRIS_EVT_DROP : TETRIS_EVT_MOVE_DOWN;
        case Action:
            return TETRIS_EVT_ROTATE;
        case Pause:
            return TETRIS_EVT_PAUSE_TOGGLE;
        case Terminate:
            return TETRIS_EVT_TERMINATE;
        case Up:
        default:
            return TETRIS_EVT_NONE;  // событие игнорируется
    }
}

/* ------------------------ Внешний интерфейс ------------------------- */

void *tetris_create(void) {
    TetrisGame *game = tetris_game_create();
    if (!game) return NULL;

    // Начальное состояние FSM
    // Предполагается, что fsm_t предоставляет API инициализации,
    // зависящее от реализации движка.
    // Например:
    // fsm_init(&game->fsm, TETRIS_STATE_INIT);

    return game;
}

void tetris_destroy(void *game) {
    if (!game) return;
    tetris_game_destroy((TetrisGame *)game);
}

void tetris_handle_input(void *g, UserAction_t action, bool hold) {
    TetrisGame *game = (TetrisGame *)g;
    if (!game) return;

    TetrisEvent evt = tetris_map_action_to_event(action, hold);
    if (!evt) return;  // игнорируем неопознанные/пустые события

    tetris_fsm_dispatch(game, evt);
}

void tetris_update(void *g) {
    TetrisGame *game = (TetrisGame *)g;
    if (!game) return;

    // Игровой тик всегда превращаем в событие FSM
    tetris_fsm_dispatch(game, TETRIS_EVT_TICK);
}

/**
 * @internal
 * @brief Обновляет структуру информации для отображения на основе текущего состояния игры
 *
 * Функция синхронизирует содержимое внутреннего игрового поля и активной фигуры
 * с публичной структурой GameInfo_t, которая используется движком для отрисовки.
 * Копирует данные из внутреннего хранилища и накладывает текущую падающую фигуру.
 *
 * @param[in] game Указатель на экземпляр игры Тетрис. Если равен NULL, функция немедленно завершается.
 *
 * @details
 * Процесс обновления включает следующие шаги:
 * 1. Копирование зафиксированных блоков из game->field_storage в game->info.field.
 * 2. Проверка состояния игры: если игра окончена или на паузе — активная фигура не отображается.
 * 3. Проверка FSM-состояния: в состояниях инициализации и "game over" фигура не рисуется.
 * 4. Получение формы текущей фигуры по её типу и повороту.
 * 5. Наложение блоков фигуры на поле отображения с проверкой выхода за границы.
 *
 * @note
 * - Функция не выделяет память — работает с уже выделёнными буферами.
 * - Значения в game->info.field устанавливаются в соответствии с цветами/типами блоков
 *   (shape[i][j] — ненулевое значение интерпретируется как активный блок).
 * - Выход за пределы поля (например, при спавне) корректно обрабатывается.
 *
 * @warning Функция предполагает, что поля game->field_storage и game->info.field
 *          уже выделены и имеют размер TETRIS_FIELD_ROWS × TETRIS_FIELD_COLS.
 *          Передача некорректного указателя может привести к segmentation fault.
 *
 * @see TetrisGame, GameInfo_t, tetris_getPieceData(), TetrisState
 */
static void tetris_update_info_view(TetrisGame *game) {
    if (!game) return;
    if (!game->field_storage || !game->info.field) return;
    // Сначала копируем зафиксированные блоки
    for (int i = 0; i < TETRIS_FIELD_ROWS; i++) {
        for (int j = 0; j < TETRIS_FIELD_COLS; j++) {
            game->info.field[i][j] = game->field_storage[i][j];
        }
    }
    // Если нет активной фигуры или игра в паузе/окончена — не рисуем
    if (game->game_over || game->info.pause) return;

    TetrisState state = (TetrisState)game->fsm.current;
    if (state == TETRIS_STATE_INIT || state == TETRIS_STATE_GAME_OVER) return;

    TetrisPiece *cur = &game->current;
    const int (*shape)[4] = tetris_getPieceData(cur->type, cur->rotation);
    if (!shape) return;

    // Накладываем текущую фигуру
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shape[i][j]) {
                int y = cur->y + i;
                int x = cur->x + j;
                if (y >= 0 && y < TETRIS_FIELD_ROWS && x >= 0 && x < TETRIS_FIELD_COLS) {
                    game->info.field[y][x] = shape[i][j];
                }
            }
        }
    }
}

const GameInfo_t *tetris_get_info(const void *g) {
    TetrisGame *game = (TetrisGame *)g;
    if (!game) return NULL;
    tetris_update_info_view(game);
    return &game->info;
}

/* ---------------- Регистрация интерфейса игры ----------------------- */

GameInterface_t tetris_get_interface(GameId_t id) {
    GameInterface_t iface = {0};
    if (id == GAME_TETRIS) {
    iface.id        = id;
    iface.create    = tetris_create;
    iface.destroy   = tetris_destroy;
    iface.input     = tetris_handle_input;
    iface.update    = tetris_update;
    iface.get_info  = tetris_get_info;
    }

    return iface;
}