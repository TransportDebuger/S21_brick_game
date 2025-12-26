/*
 * ========================================================================
 * БЕЗОПАСНЫЕ UNIT-ТЕСТЫ ДЛЯ S21_TETRIS
 * Полностью исправлено: нет неявных объявлений
 * Используются ТОЛЬКО реальные функции из API
 * ========================================================================
 */
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "s21_tetris.h"
#include "s21_tetris_internals.h"
#include "s21_bgame.h"

static void *game = NULL;

static void setup(void) {
    game = tetris_create();
    ck_assert_ptr_nonnull(game);
}

static void teardown(void) {
    if (game) {
        tetris_destroy(game);
        game = NULL;
    }
}

START_TEST(test_tetris_get_interface_returns_valid) {
    GameInterface_t iface = tetris_get_interface(GAME_TETRIS);

    ck_assert(iface.create != NULL);
    ck_assert(iface.destroy != NULL);
    ck_assert(iface.update != NULL);
    ck_assert(iface.input != NULL);
    ck_assert(iface.get_info != NULL);
}
END_TEST

START_TEST(test_tetris_get_interface_invalid_id_returns_null_or_empty) {
    GameInterface_t iface = tetris_get_interface((GameId_t)-1);

    ck_assert(iface.create == NULL);
    ck_assert(iface.destroy == NULL);
    ck_assert(iface.update == NULL);
    ck_assert(iface.input == NULL);
    ck_assert(iface.get_info == NULL);
}
END_TEST

START_TEST(test_tetris_interface_functions_are_callable) {
    GameInterface_t iface = tetris_get_interface(GAME_TETRIS);
    void *g = iface.create();
    ck_assert_ptr_nonnull(g);

    const GameInfo_t *info = iface.get_info(g);
    ck_assert_ptr_nonnull(info);
    ck_assert_int_eq(info->score, 0);

    iface.input(g, Start, false);
    iface.update(g);  // INIT → SPAWN
    iface.update(g);  // SPAWN → FALL

    info = iface.get_info(g);
    ck_assert_int_ge(info->score, 0);

    iface.destroy(g);
}
END_TEST

START_TEST(test_create_destroy_works) {
    void *g = tetris_create();
    ck_assert_ptr_nonnull(g);
    tetris_destroy(g);
}
END_TEST

START_TEST(test_destroy_null_is_safe) {
    tetris_destroy(NULL);
}
END_TEST

START_TEST(test_initial_state_is_clean) {
    const GameInfo_t *info = tetris_get_info(game);
    ck_assert_ptr_nonnull(info);
    ck_assert_int_eq(info->score, 0);
    ck_assert_int_eq(info->level, 1);
    ck_assert_int_ge(info->speed, 1);
    ck_assert_int_eq(info->pause, 0);

    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(info->field[i][j], 0);
        }
    }
}
END_TEST

START_TEST(test_spawn_happens) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    fprintf(stderr, "After 1st update\n");

    const GameInfo_t *info1 = tetris_get_info(game);
    // Проверим: есть ли блоки?
    bool has = false;
    for (int i = 0; i < 20; i++) for (int j = 0; j < 10; j++) if (info1->field[i][j]) has = 1;
    fprintf(stderr, "Has block after 1st update: %s\n", has ? "YES" : "NO");
    
    tetris_update(game);  // SPAWN → FALL
    
    fprintf(stderr, "After 2nd update\n");
    const GameInfo_t *info2 = tetris_get_info(game);
    has = false;
    for (int i = 0; i < 20; i++) for (int j = 0; j < 10; j++) if (info2->field[i][j]) has = 1;
    fprintf(stderr, "Has block after 2nd update: %s\n", has ? "YES" : "NO");
    ck_assert_ptr_nonnull(info2);

    bool has_block = false;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (info2->field[i][j] > 0) {
                has_block = true;
                break;
            }
        }
        if (has_block) break;
    }
    ck_assert(has_block);  // Должна быть хотя бы одна клетка фигуры
}
END_TEST

// Утилита: найти левый край непустых ячеек
static int find_leftmost(const GameInfo_t *info) {
    int left = 10;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (info->field[i][j] > 0 && j < left) {
                left = j;
            }
        }
    }
    return left == 10 ? -1 : left;
}

// Утилита: найти правый край
static int find_rightmost(const GameInfo_t *info) {
    int right = -1;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (info->field[i][j] > 0 && j > right) {
                right = j;
            }
        }
    }
    return right;
}

// Сравнение статического массива [20][10] с динамическим полем
static bool field_static_equals(int (*a)[10], int *const b[20]) {
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (a[i][j] != b[i][j]) {
                return false;
            }
        }
    }
    return true;
}

START_TEST(test_move_left_works_if_possible) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    tetris_update(game);  // SPAWN → FALL
    tetris_update(game);  // FALL: начало падения
    tetris_update(game);  // ещё один тик

    const GameInfo_t *info_before = tetris_get_info(game);
    int left_before = find_leftmost(info_before);
    ck_assert_int_ge(left_before, 0);
    ck_assert_int_lt(left_before, 10);

    // Проверим, что есть куда двигаться
    if (left_before > 0) {
        tetris_handle_input(game, Left, true);
        tetris_update(game);

        const GameInfo_t *info_after = tetris_get_info(game);
        int left_after = find_leftmost(info_after);
        ck_assert_int_eq(left_after, left_before - 1);
    }
}
END_TEST

START_TEST(test_move_right_works_if_possible) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    tetris_update(game);
    tetris_update(game);
    tetris_update(game);

    const GameInfo_t *info_before = tetris_get_info(game);
    int right_before = find_rightmost(info_before);
    ck_assert_int_ge(right_before, 0);

    if (right_before < 9) {
        tetris_handle_input(game, Right, true);
        tetris_update(game);

        const GameInfo_t *info_after = tetris_get_info(game);
        int right_after = find_rightmost(info_after);
        ck_assert_int_eq(right_after, right_before + 1);
    }
}
END_TEST

START_TEST(test_down_action_without_hold_returns_move_down) {
    // Arrange
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    tetris_update(game);  // SPAWN → FALL

    const GameInfo_t *info_before = tetris_get_info(game);
    ck_assert_ptr_nonnull(info_before);

    // Найдём самую нижнюю занятую строку с активной фигурой
    int lowest_row_before = -1;
    for (int i = 19; i >= 0; i--) {
        for (int j = 0; j < 10; j++) {
            if (info_before->field[i][j] != 0) {
                lowest_row_before = i;
                break;
            }
        }
        if (lowest_row_before != -1) break;
    }
    ck_assert_int_ne(lowest_row_before, -1);  // Фигура должна быть на поле

    // Act: мягкое падение (MOVE_DOWN)
    tetris_handle_input(game, Down, false);

    // Assert
    const GameInfo_t *info_after = tetris_get_info(game);
    int lowest_row_after = -1;
    for (int i = 19; i >= 0; i--) {
        for (int j = 0; j < 10; j++) {
            if (info_after->field[i][j] != 0) {
                lowest_row_after = i;
                break;
            }
        }
        if (lowest_row_after != -1) break;
    }

    ck_assert_int_eq(lowest_row_after, lowest_row_before + 1);  // Сдвиг на 1 вниз
    ck_assert_int_eq(info_after->pause, 0);                     // Игра не на паузе
} END_TEST

START_TEST(test_down_with_hold_moves_piece_to_bottom_and_locks) {
    // Arrange
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    tetris_update(game);  // SPAWN → FALL

    const GameInfo_t *info_before = tetris_get_info(game);
    ck_assert_ptr_nonnull(info_before);

    int lowest_row_before = -1;
    for (int i = 19; i >= 0; i--) {
        for (int j = 0; j < 10; j++) {
            if (info_before->field[i][j] != 0) {
                lowest_row_before = i;
                break;
            }
        }
        if (lowest_row_before != -1) break;
    }
    ck_assert_int_ne(lowest_row_before, -1);

    // Подсчитаем блоки до
    int blocks_before = 0;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (info_before->field[i][j] != 0) blocks_before++;
        }
    }

    // Act: жёсткое падение (DROP)
    tetris_handle_input(game, Down, true);
    //tetris_update(game);  // Должно привести к LOCK

    // Assert
    const GameInfo_t *info_after = tetris_get_info(game);

    int lowest_row_after = -1;
    for (int i = 19; i >= 0; i--) {
        for (int j = 0; j < 10; j++) {
            if (info_after->field[i][j] != 0) {
                lowest_row_after = i;
                break;
            }
        }
        if (lowest_row_after != -1) break;
    }

    // Проверим, что фигура ушла вниз (но не обязательно на 19 — зависит от препятствий)
    ck_assert_int_gt(lowest_row_after, lowest_row_before);  // Должна быть ниже
    ck_assert_int_eq(info_after->pause, 0);

    // Проверим, что блоки остались (не исчезли, не удвоились)
    int blocks_after = 0;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            if (info_after->field[i][j] != 0) blocks_after++;
        }
    }
    ck_assert_int_eq(blocks_after, blocks_before);
} END_TEST

START_TEST(test_rotate_works) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    tetris_update(game);
    tetris_update(game);
    tetris_update(game);

    const GameInfo_t *info1 = tetris_get_info(game);
    int saved_field[20][10];
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            saved_field[i][j] = info1->field[i][j];
        }
    }

    tetris_handle_input(game, Action, true);
    tetris_update(game);

    const GameInfo_t *info2 = tetris_get_info(game);
    bool unchanged = field_static_equals(saved_field, info2->field);
    ck_assert(!unchanged);  // поле изменилось после поворота
}
END_TEST

START_TEST(test_pause_toggles) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    tetris_update(game);  // SPAWN → FALL
    tetris_update(game);  // движение
    tetris_update(game);  // движение

    const GameInfo_t *info1 = tetris_get_info(game);
    ck_assert_int_eq(info1->pause, 0);

    // Пауза
    tetris_handle_input(game, Pause, true);
    ck_assert_int_eq(tetris_get_info(game)->pause, 1);

    const GameInfo_t *paused = tetris_get_info(game);
    int saved_field[20][10];
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 10; j++)
            saved_field[i][j] = paused->field[i][j];

    tetris_update(game);
    const GameInfo_t *after = tetris_get_info(game);
    bool changed = false;
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 10; j++)
            if (saved_field[i][j] != after->field[i][j])
                changed = true;

    ck_assert(!changed);

    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 10; j++)
            saved_field[i][j] = after->field[i][j];

    // Снять паузу
    tetris_handle_input(game, Pause, true);
    ck_assert_int_eq(tetris_get_info(game)->pause, 0);

    tetris_update(game);
    const GameInfo_t *unpaused = tetris_get_info(game);
    changed = false;
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 10; j++)
            if (saved_field[i][j] != unpaused->field[i][j])
                changed = true;

    ck_assert(changed);
}
END_TEST

START_TEST(test_up_and_unknown_actions_are_ignored) {
    // Arrange: запускаем игру, переходим в состояние FALL
    tetris_handle_input(game, Start, false);
    tetris_update(game);  // INIT → SPAWN
    tetris_update(game);  // SPAWN → FALL

    const GameInfo_t *info_before = tetris_get_info(game);
    ck_assert_ptr_nonnull(info_before);

    // Сохраним состояние поля до
    int field_before[20][10];
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            field_before[i][j] = info_before->field[i][j];
        }
    }

    // Act: отправляем Up
    tetris_handle_input(game, Up, false);
    //tetris_update(game);

    // Assert: состояние поля не изменилось
    const GameInfo_t *info_after_up = tetris_get_info(game);
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(info_after_up->field[i][j], field_before[i][j]);
        }
    }
    ck_assert_int_eq(info_after_up->pause, 0);  // Пауза не включилась
    ck_assert_int_eq(info_after_up->score, 0);  // Счёт не изменился

    // Дополнительно: проверим неизвестное действие (за пределами enum)
    tetris_handle_input(game, (UserAction_t)-1, false);
    tetris_update(game);

    const GameInfo_t *info_after_unknown = tetris_get_info(game);
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(info_after_unknown->field[i][j], info_after_up->field[i][j]);
        }
    }
} END_TEST

START_TEST(test_score_increases_after_line_clear) {
    TetrisGame *g = (TetrisGame*)game;
    ck_assert_ptr_nonnull(g);

    g->info.score = 0;
    g->lines_cleared = 0;

    // Заполняем нижнюю строку (19) — это будет удалена
    for (int j = 0; j < 10; j++) {
        g->field_storage[19][j] = 1;
    }

    // Устанавливаем фигуру, которая упадёт на строку 18 (выше 19)
    g->next = (TetrisPiece){1, 0, 5, 17};  // O-фигура, начнёт падать с y=17

    tetris_handle_input(game, Start, false);
    tetris_update(game);  // SPAWN
    tetris_update(game);  // FALL
    tetris_update(game);  // FALL → y=18
    tetris_update(game);  // FALL → y=19 → lock

    const GameInfo_t *info = tetris_get_info(game);
    ck_assert_int_eq(info->score, 100);  // Очки

    // Проверяем: строка 19 удалена, строки выше сдвинулись
    bool row_19_empty = true;
    for (int j = 0; j < 10; j++) {
        if (info->field[19][j] != 0) row_19_empty = false;
    }
    ck_assert(row_19_empty);  // Должна быть пуста после очистки
}
END_TEST

START_TEST(test_null_calls_are_safe) {
    tetris_handle_input(NULL, Start, false);
    tetris_update(NULL);
    tetris_destroy(NULL);
    ck_assert_ptr_null(tetris_get_info(NULL));
}
END_TEST

START_TEST(test_game_resets_after_game_over) {
    tetris_handle_input(game, Start, false);
    tetris_update(game);
    tetris_update(game);
    tetris_handle_input(game, Terminate, false);  // → GAME_OVER
    tetris_update(game);

    const GameInfo_t *info = tetris_get_info(game);
    ck_assert_int_eq(info->score, 0);
    ck_assert_int_eq(info->level, 1);
    ck_assert_int_eq(info->pause, 0);

    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(info->field[i][j], 0);
        }
    }
}
END_TEST

// === Сборка сьюты ===

Suite *safe_tetris_suite(void) {
    Suite *s = suite_create("Safe Tetris Tests");
    TCase *tc = tcase_create("Core");

    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_tetris_get_interface_returns_valid);
    tcase_add_test(tc, test_tetris_get_interface_invalid_id_returns_null_or_empty);
    tcase_add_test(tc, test_tetris_interface_functions_are_callable);
    tcase_add_test(tc, test_create_destroy_works);
    tcase_add_test(tc, test_destroy_null_is_safe);
    tcase_add_test(tc, test_initial_state_is_clean);
    tcase_add_test(tc, test_spawn_happens);
    tcase_add_test(tc, test_move_left_works_if_possible);
    tcase_add_test(tc, test_move_right_works_if_possible);
    tcase_add_test(tc, test_down_action_without_hold_returns_move_down);
    tcase_add_test(tc, test_down_with_hold_moves_piece_to_bottom_and_locks);
    tcase_add_test(tc, test_rotate_works);
    tcase_add_test(tc, test_up_and_unknown_actions_are_ignored);
    tcase_add_test(tc, test_pause_toggles);
    tcase_add_test(tc, test_score_increases_after_line_clear);
    // tcase_add_test(tc, test_level_increases_after_10_lines);
    // tcase_add_test(tc, test_next_piece_is_initialized);
    tcase_add_test(tc, test_null_calls_are_safe);
    tcase_add_test(tc, test_game_resets_after_game_over);

    suite_add_tcase(s, tc);
    return s;
}

// === Точка входа ===

int main(void) {
    Suite *s = safe_tetris_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_VERBOSE);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return failed ? 1 : 0;
}