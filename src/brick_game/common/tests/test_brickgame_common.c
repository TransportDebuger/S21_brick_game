/**
 * @file test_brickgame_common.c
 * @brief Unit tests for BrickGame common utilities
 *
 * Tests all functions in brickgame_common.h using the check library.
 * Covers:
 * - Memory allocation and deallocation
 * - Memory clearing
 * - High score persistence
 * - GameInfo initialization and cleanup
 * - Validation functions
 *
 * Compilation:
 *   gcc -Wall -Wextra -std=c11 test_brickgame_common.c ../brickgame_common.c \
 *       -o test_brickgame_common -lcheck -lm -lsubunit
 *
 * Execution:
 *   ./test_brickgame_common
 *
 * @author BrickGame Project
 * @version 1.0
 * @date 2025-12-16
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "s21_bgame_cmn.h"

/* ============================================================================
 * MEMORY ALLOCATION TESTS - FIELD
 * ============================================================================ */

START_TEST(test_allocate_field_success) {
    int **field = brickgame_allocate_field();

    ck_assert_ptr_nonnull(field);
    ck_assert_ptr_nonnull(field[0]);
    ck_assert_ptr_nonnull(field[19]);

    // Check initialization to 0
    ck_assert_int_eq(field[0][0], 0);
    ck_assert_int_eq(field[10][5], 0);
    ck_assert_int_eq(field[19][9], 0);

    brickgame_free_field(field);
}
END_TEST

START_TEST(test_allocate_field_all_rows) {
    int **field = brickgame_allocate_field();
    ck_assert_ptr_nonnull(field);

    // Verify all rows are allocated
    for (int i = 0; i < 20; i++) {
        ck_assert_ptr_nonnull(field[i]);
    }

    brickgame_free_field(field);
}
END_TEST

START_TEST(test_free_field_null_safe) {
    // Should not crash with NULL
    brickgame_free_field(NULL);
    ck_assert(1);  // If we reach here, test passed
}
END_TEST

START_TEST(test_clear_field) {
    int **field = brickgame_allocate_field();

    // Set some cells to 1
    field[0][0] = 1;
    field[10][5] = 1;
    field[19][9] = 1;

    // Clear field
    brickgame_clear_field(field);

    // Verify all cells are 0
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(field[i][j], 0);
        }
    }

    brickgame_free_field(field);
}
END_TEST

START_TEST(test_clear_field_null_safe) {
    // Should not crash with NULL
    brickgame_clear_field(NULL);
    ck_assert(1);
}
END_TEST

/* ============================================================================
 * MEMORY ALLOCATION TESTS - NEXT PREVIEW
 * ============================================================================ */

START_TEST(test_allocate_next_success) {
    int **next = brickgame_allocate_next();

    ck_assert_ptr_nonnull(next);
    ck_assert_ptr_nonnull(next[0]);
    ck_assert_ptr_nonnull(next[3]);

    // Check initialization to 0
    ck_assert_int_eq(next[0][0], 0);
    ck_assert_int_eq(next[2][2], 0);
    ck_assert_int_eq(next[3][3], 0);

    brickgame_free_next(next);
}
END_TEST

START_TEST(test_allocate_next_all_rows) {
    int **next = brickgame_allocate_next();
    ck_assert_ptr_nonnull(next);

    // Verify all rows are allocated
    for (int i = 0; i < 4; i++) {
        ck_assert_ptr_nonnull(next[i]);
    }

    brickgame_free_next(next);
}
END_TEST

START_TEST(test_free_next_null_safe) {
    // Should not crash with NULL
    brickgame_free_next(NULL);
    ck_assert(1);
}
END_TEST

START_TEST(test_clear_next) {
    int **next = brickgame_allocate_next();

    // Set some cells to 1
    next[0][0] = 1;
    next[2][2] = 1;
    next[3][3] = 1;

    // Clear next
    brickgame_clear_next(next);

    // Verify all cells are 0
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ck_assert_int_eq(next[i][j], 0);
        }
    }

    brickgame_free_next(next);
}
END_TEST

START_TEST(test_clear_next_null_safe) {
    // Should not crash with NULL
    brickgame_clear_next(NULL);
    ck_assert(1);
}
END_TEST

/* ============================================================================
 * HIGH SCORE PERSISTENCE TESTS
 * ============================================================================ */

START_TEST(test_save_and_load_high_score) {
    const char *game_name = "test_game_unit";
    int score = 12345;

    // Save score
    bool save_result = brickgame_save_high_score(game_name, score);
    ck_assert(save_result);

    // Load score
    int loaded_score = brickgame_load_high_score(game_name);
    ck_assert_int_eq(loaded_score, score);
}
END_TEST

START_TEST(test_save_high_score_zero) {
    const char *game_name = "test_game_zero";
    int score = 0;

    bool save_result = brickgame_save_high_score(game_name, score);
    ck_assert(save_result);

    int loaded_score = brickgame_load_high_score(game_name);
    ck_assert_int_eq(loaded_score, score);
}
END_TEST

START_TEST(test_save_high_score_large) {
    const char *game_name = "test_game_large";
    int score = 2147483647;  // Max int

    bool save_result = brickgame_save_high_score(game_name, score);
    ck_assert(save_result);

    int loaded_score = brickgame_load_high_score(game_name);
    ck_assert_int_eq(loaded_score, score);
}
END_TEST

START_TEST(test_save_high_score_invalid_negative) {
    const char *game_name = "test_game_negative";
    int score = -100;

    // Should reject negative scores
    bool save_result = brickgame_save_high_score(game_name, score);
    ck_assert(!save_result);
}
END_TEST

START_TEST(test_save_high_score_null_game_name) {
    int score = 100;

    // Should reject NULL game name
    bool save_result = brickgame_save_high_score(NULL, score);
    ck_assert(!save_result);
}
END_TEST

START_TEST(test_load_nonexistent_high_score) {
    // Load from nonexistent file should return 0
    int score = brickgame_load_high_score("nonexistent_game_xyz_123");
    ck_assert_int_eq(score, 0);
}
END_TEST

START_TEST(test_load_null_game_name) {
    // Should return 0 for NULL
    int score = brickgame_load_high_score(NULL);
    ck_assert_int_eq(score, 0);
}
END_TEST

/* ============================================================================
 * GAME INFO MANAGEMENT TESTS
 * ============================================================================ */

START_TEST(test_create_game_info_success) {
    GameInfo_t info = brickgame_create_game_info();

    // Check pointers allocated
    ck_assert_ptr_nonnull(info.field);
    ck_assert_ptr_nonnull(info.next);

    // Check default values
    ck_assert_int_eq(info.score, 0);
    ck_assert_int_eq(info.high_score, 0);
    ck_assert_int_eq(info.level, 1);
    ck_assert_int_eq(info.speed, 800);
    ck_assert_int_eq(info.pause, 0);

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_create_game_info_field_initialized) {
    GameInfo_t info = brickgame_create_game_info();

    // Check field is initialized to 0
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            ck_assert_int_eq(info.field[i][j], 0);
        }
    }

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_create_game_info_next_initialized) {
    GameInfo_t info = brickgame_create_game_info();

    // Check next is initialized to 0
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ck_assert_int_eq(info.next[i][j], 0);
        }
    }

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_destroy_game_info_nullifies) {
    GameInfo_t info = brickgame_create_game_info();
    ck_assert_ptr_nonnull(info.field);
    ck_assert_ptr_nonnull(info.next);

    brickgame_destroy_game_info(&info);

    // After destroy, pointers should be NULL
    ck_assert_ptr_null(info.field);
    ck_assert_ptr_null(info.next);
    ck_assert_int_eq(info.score, 0);
    ck_assert_int_eq(info.level, 0);
}
END_TEST

START_TEST(test_destroy_game_info_null_safe) {
    // Should not crash with NULL
    brickgame_destroy_game_info(NULL);
    ck_assert(1);
}
END_TEST

/* ============================================================================
 * VALIDATION TESTS - ACTION
 * ============================================================================ */

START_TEST(test_is_valid_action_all_valid) {
    ck_assert(brickgame_is_valid_action(Start));
    ck_assert(brickgame_is_valid_action(Pause));
    ck_assert(brickgame_is_valid_action(Terminate));
    ck_assert(brickgame_is_valid_action(Left));
    ck_assert(brickgame_is_valid_action(Right));
    ck_assert(brickgame_is_valid_action(Up));
    ck_assert(brickgame_is_valid_action(Down));
    ck_assert(brickgame_is_valid_action(Action));
}
END_TEST

/* ============================================================================
 * VALIDATION TESTS - FIELD
 * ============================================================================ */

START_TEST(test_is_valid_field_valid) {
    int **field = brickgame_allocate_field();
    ck_assert(brickgame_is_valid_field(field));

    field[0][0] = 1;
    ck_assert(brickgame_is_valid_field(field));

    brickgame_free_field(field);
}
END_TEST

START_TEST(test_is_valid_field_null) {
    ck_assert(!brickgame_is_valid_field(NULL));
}
END_TEST

START_TEST(test_is_valid_field_invalid_cell_value) {
    int **field = brickgame_allocate_field();

    // Set invalid value
    field[0][0] = 2;
    ck_assert(!brickgame_is_valid_field(field));

    field[0][0] = -1;
    ck_assert(!brickgame_is_valid_field(field));

    brickgame_free_field(field);
}
END_TEST

/* ============================================================================
 * VALIDATION TESTS - GAME INFO
 * ============================================================================ */

START_TEST(test_is_valid_game_info_valid) {
    GameInfo_t info = brickgame_create_game_info();
    ck_assert(brickgame_is_valid_game_info(&info));

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_is_valid_game_info_null) {
    ck_assert(!brickgame_is_valid_game_info(NULL));
}
END_TEST

START_TEST(test_is_valid_game_info_invalid_level) {
    GameInfo_t info = brickgame_create_game_info();

    // Valid range is [1, 10]
    info.level = 0;
    ck_assert(!brickgame_is_valid_game_info(&info));

    info.level = 11;
    ck_assert(!brickgame_is_valid_game_info(&info));

    info.level = -1;
    ck_assert(!brickgame_is_valid_game_info(&info));

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_is_valid_game_info_invalid_pause) {
    GameInfo_t info = brickgame_create_game_info();

    // Valid values are 0 or 1
    info.pause = 2;
    ck_assert(!brickgame_is_valid_game_info(&info));

    info.pause = -1;
    ck_assert(!brickgame_is_valid_game_info(&info));

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_is_valid_game_info_invalid_score) {
    GameInfo_t info = brickgame_create_game_info();

    // Score must be non-negative
    info.score = -1;
    ck_assert(!brickgame_is_valid_game_info(&info));

    brickgame_destroy_game_info(&info);
}
END_TEST

START_TEST(test_is_valid_game_info_invalid_speed) {
    GameInfo_t info = brickgame_create_game_info();

    // Speed must be non-negative
    info.speed = -1;
    ck_assert(!brickgame_is_valid_game_info(&info));

    brickgame_destroy_game_info(&info);
}
END_TEST

/* ============================================================================
 * TEST SUITE SETUP
 * ============================================================================ */

Suite *brickgame_common_suite(void) {
    Suite *s = suite_create("brickgame_common");

    // Field allocation tests
    TCase *tc_field = tcase_create("field_allocation");
    tcase_add_test(tc_field, test_allocate_field_success);
    tcase_add_test(tc_field, test_allocate_field_all_rows);
    tcase_add_test(tc_field, test_free_field_null_safe);
    tcase_add_test(tc_field, test_clear_field);
    tcase_add_test(tc_field, test_clear_field_null_safe);
    suite_add_tcase(s, tc_field);

    // Next preview allocation tests
    TCase *tc_next = tcase_create("next_allocation");
    tcase_add_test(tc_next, test_allocate_next_success);
    tcase_add_test(tc_next, test_allocate_next_all_rows);
    tcase_add_test(tc_next, test_free_next_null_safe);
    tcase_add_test(tc_next, test_clear_next);
    tcase_add_test(tc_next, test_clear_next_null_safe);
    suite_add_tcase(s, tc_next);

    // High score persistence tests
    TCase *tc_score = tcase_create("high_score");
    tcase_add_test(tc_score, test_save_and_load_high_score);
    tcase_add_test(tc_score, test_save_high_score_zero);
    tcase_add_test(tc_score, test_save_high_score_large);
    tcase_add_test(tc_score, test_save_high_score_invalid_negative);
    tcase_add_test(tc_score, test_save_high_score_null_game_name);
    tcase_add_test(tc_score, test_load_nonexistent_high_score);
    tcase_add_test(tc_score, test_load_null_game_name);
    suite_add_tcase(s, tc_score);

    // GameInfo management tests
    TCase *tc_gameinfo = tcase_create("gameinfo");
    tcase_add_test(tc_gameinfo, test_create_game_info_success);
    tcase_add_test(tc_gameinfo, test_create_game_info_field_initialized);
    tcase_add_test(tc_gameinfo, test_create_game_info_next_initialized);
    tcase_add_test(tc_gameinfo, test_destroy_game_info_nullifies);
    tcase_add_test(tc_gameinfo, test_destroy_game_info_null_safe);
    suite_add_tcase(s, tc_gameinfo);

    // Validation tests
    TCase *tc_validation = tcase_create("validation");
    tcase_add_test(tc_validation, test_is_valid_action_all_valid);
    tcase_add_test(tc_validation, test_is_valid_field_valid);
    tcase_add_test(tc_validation, test_is_valid_field_null);
    tcase_add_test(tc_validation, test_is_valid_field_invalid_cell_value);
    tcase_add_test(tc_validation, test_is_valid_game_info_valid);
    tcase_add_test(tc_validation, test_is_valid_game_info_null);
    tcase_add_test(tc_validation, test_is_valid_game_info_invalid_level);
    tcase_add_test(tc_validation, test_is_valid_game_info_invalid_pause);
    tcase_add_test(tc_validation, test_is_valid_game_info_invalid_score);
    tcase_add_test(tc_validation, test_is_valid_game_info_invalid_speed);
    suite_add_tcase(s, tc_validation);

    return s;
}

int main(void) {
    Suite *s = brickgame_common_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}