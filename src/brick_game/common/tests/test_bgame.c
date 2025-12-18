/**
 * @file test_bgame.c
 * @brief Модульные тесты для игрового фреймворка s21_bgame
 *
 * Содержит набор unit-тестов, проверяющих корректность работы основных функций
 * игрового фреймворка: регистрация игр, переключение между ними, обработка ввода
 * и обновление состояния. Используется фреймворк CMocka для организации тестов.
 *
 * Особое внимание уделено изоляции тестов: перед каждым тестом состояние
 * фреймворка сбрасывается с помощью bg_reset_current_for_testing(), доступной
 * только при сборке с флагом -DTEST_ENV.
 *
 * @note Тесты предполагают, что s21_bgame работает в однопоточном режиме.
 * @author GigaCode
 * @date 2025
 * @see s21_bgame.h, s21_bgame.c
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "s21_bgame.h"

#define BG_MAX_GAMES 8

// ============= Заглушки для тестов =============

/**
 * @brief Простая заглушка экземпляра игры для тестирования
 *
 * Структура используется как минимальная реализация игрового состояния
 * для проверки интерфейса GameInterface_t. Содержит только идентификатор
 * для отслеживания изменений в процессе тестов.
 */
typedef struct {
    int id; ///< Уникальный идентификатор экземпляра (для отладки)
} TestGame;

/**
 * @brief Заглушка функции create для тестов
 *
 * Имитирует создание экземпляра игры. Выделяет память под TestGame
 * и инициализирует её начальным значением id = 42.
 *
 * @return Указатель на новый экземпляр игры или NULL при неудаче
 *
 * @note В случае ошибки выделения памяти возвращает NULL.
 * @warning Выделенная память должна быть освобождена через test_destroy().
 */
static void *test_create(void) {
    TestGame *game = (TestGame *)malloc(sizeof(TestGame));
    if (game) {
        game->id = 42;
    }
    return game;
}

/**
 * @brief Заглушка функции destroy для тестов
 *
 * Имитирует уничтожение экземпляра игры. Освобождает память,
 * выделенную через test_create().
 *
 * @param game Указатель на экземпляр игры (может быть NULL)
 *
 * @note Безопасно обрабатывает NULL-указатель.
 */
static void test_destroy(void *game) {
    free(game);
}

/**
 * @brief Заглушка функции input для тестов
 *
 * Проверяет, что функция userInput() корректно передаёт действия
 * в интерфейс игры. Использует макросы CMocka для ожидания аргументов.
 *
 * @param game Указатель на экземпляр игры (не используется)
 * @param action Ожидаемое пользовательское действие
 * @param hold Признак удержания клавиши
 *
 * @note (void)game — подавление предупреждения неиспользованной переменной.
 * @see expect_value(), userInput()
 */
static void test_input(void *game, UserAction_t action, bool hold) {
    check_expected(action);
    check_expected(hold);
    (void)game;
}

/**
 * @brief Заглушка функции update для тестов
 *
 * Имитирует игровой тик: увеличивает поле id экземпляра игры на 1.
 * Используется для проверки, что updateCurrentState вызывает update.
 *
 * @param game Указатель на экземпляр игры (ожидается TestGame*)
 */
static void test_update(void *game) {
    TestGame *g = (TestGame *)game;
    g->id++;
}

/**
 * @brief Заглушка функции get_info для тестов
 *
 * Возвращает фиксированное состояние игры для проверки
 * корректности updateCurrentState(). Использует статические данные
 * для обеспечения стабильности указателей.
 *
 * @param game Указатель на экземпляр игры (используется для score)
 * @return Указатель на статическую структуру GameInfo_t
 *
 * @note Возвращает указатель на статические данные — не требует освобождения.
 * @warning Не является потокобезопасной из-за использования static.
 */
static const GameInfo_t *test_get_info(const void *game) {
    static GameInfo_t info = {0};
    const TestGame *g = (const TestGame *)game;

    info.score = g->id;
    info.pause = 0;
    info.level = 1;
    info.speed = 500;

    // Имитация игрового поля 2x2
    static int field_data[2][2] = {{1, 0}, {0, 1}};
    static int *field_rows[2] = {field_data[0], field_data[1]};
    info.field = field_rows;

    // Имитация next
    static int next_data[2][2] = {{1, 1}, {0, 0}};
    static int *next_rows[2] = {next_data[0], next_data[1]};
    info.next = next_rows;

    return &info;
}

// ============= Сброс состояния между тестами =============

/**
 * @brief Сбрасывает глобальное состояние фреймворка
 *
 * Уничтожает текущий экземпляр игры (если он есть) и сбрасывает контекст
 * через bg_reset_current_for_testing(). Вызывается после каждого теста.
 *
 * @param state Указатель на состояние теста (не используется)
 * @return Всегда 0 (успех)
 *
 * @note Требует, чтобы s21_bgame.c определял bg_reset_current_for_testing()
 *       при компиляции с -DTEST_ENV.
 */
static int teardown(void **state) {
    (void)state;

    if (bg_get_current_instance()) {
        const GameInterface_t *iface = bg_get_current_game();
        if (iface && iface->destroy) {
            iface->destroy(bg_get_current_instance());
        }
    }

    bg_reset_current_for_testing();
    bg_reset_registry_for_testing();
    
    return 0;
}

/**
 * @brief Подготовка перед каждым тестом
 *
 * Пока не требуется, но нужно для cmocka.
 *
 * @param state Указатель на состояние теста (не используется)
 * @return Всегда 0 (успех)
 */
static int setup(void **state) {
    (void)state;
    return 0;
}

// ============= Тесты =============

/**
 * @brief Тест: регистрация и получение игры по ID
 *
 * Проверяет, что:
 * - bg_register_game() корректно добавляет игру в реестр
 * - bg_get_game() возвращает правильный интерфейс по ID
 * - Поля интерфейса сохраняются без изменений
 */
static void test_bg_register_and_get_game(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_TETRIS,
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    const GameInterface_t *found = bg_get_game(GAME_TETRIS);

    assert_non_null(found);
    assert_int_equal(found->id, GAME_TETRIS);
    assert_ptr_equal(found->create, test_create);
}

/**
 * @brief Тест: получение незарегистрированной игры
 *
 * Проверяет, что bg_get_game() возвращает NULL,
 * если игра с указанным ID не была зарегистрирована.
 */
static void test_bg_get_game_not_registered(void **state) {
    (void)state;
    const GameInterface_t *found = bg_get_game(GAME_SNAKE);
    assert_null(found);
}

/**
 * @brief Тест: успешное переключение на игру
 *
 * Проверяет, что:
 * - bg_switch_game() возвращает true при успехе
 * - Устанавливается корректный интерфейс и экземпляр
 * - Вызывается create()
 */
static void test_bg_switch_game_success(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_TETRIS,
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    bool result = bg_switch_game(GAME_TETRIS);

    assert_true(result);
    assert_non_null(bg_get_current_game());
    assert_non_null(bg_get_current_instance());
}

/**
 * @brief Тест: переключение на игру с некорректным интерфейсом
 *
 * Проверяет, что:
 * - bg_switch_game() возвращает false при NULL create/destroy
 * - Текущая игра не устанавливается
 * - g_current.instance и g_current.iface остаются NULL
 */
static void test_bg_switch_game_invalid_interface(void **state) {
    (void)state;

    GameInterface_t bad_iface = {
        .id = GAME_SNAKE,
        .create = NULL,
        .destroy = NULL,
        .input = NULL,
        .update = NULL,
        .get_info = NULL
    };

    bg_register_game(bad_iface);
    bool result = bg_switch_game(GAME_SNAKE);

    assert_false(result);
    assert_null(bg_get_current_game());
    assert_null(bg_get_current_instance());
}

/**
 * @brief Тест: повторное переключение на ту же игру
 *
 * Проверяет, что:
 * - bg_switch_game() возвращает true при повторном вызове
 * - Экземпляр игры не пересоздаётся
 * - Сохраняется согласованное состояние
 */
static void test_bg_switch_game_to_same_game(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_TETRIS,
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    bg_switch_game(GAME_TETRIS);

    void *old_instance = bg_get_current_instance();
    bool result = bg_switch_game(GAME_TETRIS);  // Повторно

    assert_true(result);
    assert_ptr_equal(bg_get_current_instance(), old_instance);  // Не пересоздан
}

/**
 * @brief Тест: передача пользовательского ввода
 *
 * Проверяет, что:
 * - userInput() вызывает input() текущей игры
 * - Аргументы (action, hold) передаются без изменений
 */
static void test_userInput_calls_input(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_TETRIS,
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    bg_switch_game(GAME_TETRIS);

    expect_value(test_input, action, Left);
    expect_value(test_input, hold, true);
    userInput(Left, true);
}

/**
 * @brief Тест: обновление состояния и получение данных
 *
 * Проверяет, что:
 * - updateCurrentState() вызывает update() и get_info()
 * - Возвращаемые данные корректны
 * - Обновления состояния сохраняются между вызовами
 */
static void test_updateCurrentState_returns_info(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_TETRIS,
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    bg_switch_game(GAME_TETRIS);

    GameInfo_t info = updateCurrentState();
    assert_non_null(info.field);
    assert_int_equal(info.score, 43);  // initial ID

    updateCurrentState();  // id++ -> 43
    info = updateCurrentState();      // id++ -> 44
    assert_int_equal(info.score, 45); // теперь точно 44
}

/**
 * @brief Тест: обновление состояния при отсутствии активной игры
 *
 * Проверяет, что updateCurrentState() возвращает обнуленную структуру,
 * если активная игра не установлена.
 */
static void test_updateCurrentState_no_game(void **state) {
    (void)state;

    GameInfo_t info = updateCurrentState();
    assert_null(info.field);
    assert_int_equal(info.score, 0);
    assert_int_equal(info.pause, 0);
}

static void test_bg_switch_game_create_fails(void **state) {
    (void)state;

    GameInterface_t iface = {
        .id = GAME_SNAKE,  // ✅
        .create = NULL,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(iface);
    bool result = bg_switch_game(GAME_SNAKE);  // ✅

    assert_false(result);
    assert_null(bg_get_current_instance());
    assert_null(bg_get_current_game());
}

static void test_bg_register_game_registry_full(void **state) {
    (void)state;

    // Регистрируем 8 игр с ID от 0 до 7
    for (int i = 0; i < BG_MAX_GAMES; ++i) {
        GameInterface_t iface = {
            .id = (GameId_t)i,
            .create = test_create,
            .destroy = test_destroy,
            .input = test_input,
            .update = test_update,
            .get_info = test_get_info
        };
        bg_register_game(iface);
    }

    // Попытка добавить ещё одну (даже с уникальным ID) — должна быть проигнорирована
    GameInterface_t extra_iface = {
        .id = (GameId_t)99,  // Уникальный ID
        .create = test_create,
        .destroy = test_destroy,
        .input = test_input,
        .update = test_update,
        .get_info = test_get_info
    };

    bg_register_game(extra_iface);

    // Убедимся, что игра с ID=99 НЕ появилась
    const GameInterface_t *overflowed = bg_get_game(99);
    assert_null(overflowed);

    // И что первая игра (id=0) осталась на месте
    const GameInterface_t *check = bg_get_game(0);
    assert_non_null(check);
    assert_ptr_equal(check->create, test_create);
}

// ============= Запуск тестов =============

/**
 * @brief Точка входа для запуска всех тестов
 *
 * Создаёт массив тестов, оборачивает их в setup/teardown
 * и передаёт в cmocka_run_group_tests() для выполнения.
 *
 * @return Код завершения: 0 — успех, ненулевое значение — ошибка
 *
 * @note Собирай с: gcc -DTEST_ENV -o test_bgame test_bgame.c s21_bgame.c -lcmocka
 */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_bg_register_and_get_game, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_get_game_not_registered, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_switch_game_success, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_switch_game_invalid_interface, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_switch_game_to_same_game, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_switch_game_create_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_userInput_calls_input, setup, teardown),
        cmocka_unit_test_setup_teardown(test_updateCurrentState_returns_info, setup, teardown),
        cmocka_unit_test_setup_teardown(test_updateCurrentState_no_game, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bg_register_game_registry_full, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}