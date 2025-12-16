/**
 * @file test_fsm.c
 * @brief Unit-тесты для универсальной библиотеки FSM
 * @author provemet
 * @date December 2024
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "s21_fsm.h"

/* Тестовые состояния */
enum test_states {
    TEST_STATE_IDLE = 0,
    TEST_STATE_ACTIVE = 1,
    TEST_STATE_PAUSED = 2,
    TEST_STATE_ERROR = 3
};

/* Тестовые события */
enum test_events {
    TEST_EVENT_NONE = 0,
    TEST_EVENT_START = 1,
    TEST_EVENT_STOP = 2,
    TEST_EVENT_PAUSE = 3,
    TEST_EVENT_RESUME = 4,
    TEST_EVENT_ERROR = 5
};

/* Тестовый контекст */
typedef struct {
    int counter;
    int last_state;
    int callback_calls;
    fsm_t *fsm_ref; // Для передачи FSM в коллбэки (только для теста рекурсии)
} test_context_t;

/* Глобальные переменные для тестов */
static test_context_t test_ctx;
static fsm_t test_fsm;

/* ===============================================
 * Callback-функции для тестов
 * =============================================== */

static void on_enter_idle(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
    tc->last_state = TEST_STATE_IDLE;
}

static void on_exit_idle(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
}

static void on_enter_active(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
    tc->last_state = TEST_STATE_ACTIVE;
    tc->counter++;
}

static void on_exit_active(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
}

static void on_enter_paused(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
    tc->last_state = TEST_STATE_PAUSED;
}

static void on_enter_error(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;
    tc->last_state = TEST_STATE_ERROR;
}

/* Колбэк, который вызывает событие изнутри — для теста рекурсии */
static void on_enter_with_event_call(fsm_context_t ctx) {
    test_context_t *tc = (test_context_t*)ctx;
    tc->callback_calls++;  // ← ДОБАВИТЬ: увеличиваем счётчик
    
    // Пытаемся вызвать событие из колбэка, для теста защиты от рекурсии
    if (tc->fsm_ref) {
        bool recursive_result = fsm_process_event(tc->fsm_ref, TEST_EVENT_STOP);
        // recursive_result будет false, т.к. fsm->processing == true
        ck_assert_int_eq(recursive_result, false);  // ← Проверяем блокировку
    }
}

/* ===============================================
 * Тестовые таблицы переходов
 * =============================================== */

/* Базовая таблица переходов */
static const fsm_transition_t basic_transitions[] = {
    /* src,             event,              dst,                on_exit,        on_enter */
    {TEST_STATE_IDLE,   TEST_EVENT_START,   TEST_STATE_ACTIVE,  on_exit_idle,   on_enter_active},
    {TEST_STATE_ACTIVE, TEST_EVENT_STOP,    TEST_STATE_IDLE,    on_exit_active, on_enter_idle},
    {TEST_STATE_ACTIVE, TEST_EVENT_PAUSE,   TEST_STATE_PAUSED,  on_exit_active, on_enter_paused},
    {TEST_STATE_PAUSED, TEST_EVENT_RESUME,  TEST_STATE_ACTIVE,  NULL,           on_enter_active},
    {TEST_STATE_PAUSED, TEST_EVENT_STOP,    TEST_STATE_IDLE,    NULL,           on_enter_idle}
};

/* Таблица с автоматическими переходами */
static const fsm_transition_t auto_transitions[] = {
    {TEST_STATE_IDLE,   TEST_EVENT_START,   TEST_STATE_ACTIVE,  NULL,           on_enter_active},
    {TEST_STATE_ACTIVE, TEST_EVENT_NONE,    TEST_STATE_PAUSED,  NULL,           on_enter_paused},
    {TEST_STATE_PAUSED, TEST_EVENT_NONE,    TEST_STATE_IDLE,    NULL,           on_enter_idle}
};

/* Таблица с переходами в состояние ошибки */
static const fsm_transition_t error_transitions[] = {
    {TEST_STATE_IDLE,   TEST_EVENT_START,   TEST_STATE_ACTIVE,  NULL,           on_enter_active},
    {TEST_STATE_ACTIVE, TEST_EVENT_ERROR,   TEST_STATE_ERROR,   NULL,           on_enter_error},
    {TEST_STATE_ERROR,  TEST_EVENT_NONE,    TEST_STATE_IDLE,    NULL,           on_enter_idle}
};

/* Таблица для теста рекурсии */
static const fsm_transition_t recursion_transitions[] = {
    {TEST_STATE_IDLE,   TEST_EVENT_START,   TEST_STATE_ACTIVE,  NULL,           on_enter_with_event_call},
    {TEST_STATE_ACTIVE, TEST_EVENT_STOP,    TEST_STATE_IDLE,    NULL,           on_enter_idle}
};

/* ===============================================
 * Setup и Teardown функции
 * =============================================== */

static void setup(void) {
    test_ctx.counter = 0;
    test_ctx.last_state = -1;
    test_ctx.callback_calls = 0;
    test_ctx.fsm_ref = NULL;
}

static void teardown(void) {
    /* Очистка после каждого теста */
}

/* ===============================================
 * Тесты функции fsm_init()
 * =============================================== */

START_TEST(test_fsm_init_success) {
    bool result = fsm_init(&test_fsm, &test_ctx, basic_transitions, 
                          sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
                          TEST_STATE_IDLE);
    
    ck_assert_int_eq(result, true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
    ck_assert_ptr_eq(test_fsm.ctx, &test_ctx);
    ck_assert_ptr_eq(test_fsm.transitions, basic_transitions);
    ck_assert_int_eq(test_fsm.count, 5);
}
END_TEST

START_TEST(test_fsm_init_null_fsm) {
    bool result = fsm_init(NULL, &test_ctx, basic_transitions, 
                          sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
                          TEST_STATE_IDLE);
    
    ck_assert_int_eq(result, false);
}
END_TEST

START_TEST(test_fsm_init_null_transitions) {
    bool result = fsm_init(&test_fsm, &test_ctx, NULL, 1, TEST_STATE_IDLE);
    
    ck_assert_int_eq(result, false);
}
END_TEST

START_TEST(test_fsm_init_zero_count) {
    bool result = fsm_init(&test_fsm, &test_ctx, basic_transitions, 0, TEST_STATE_IDLE);
    
    ck_assert_int_eq(result, false);
}
END_TEST

START_TEST(test_fsm_init_callback_execution) {
    fsm_transition_t init_transitions[] = {
        {TEST_STATE_IDLE, TEST_STATE_IDLE, TEST_STATE_IDLE, NULL, on_enter_idle}
    };
    ck_assert_int_eq(fsm_init(&test_fsm, &test_ctx,
        init_transitions, 1, TEST_STATE_IDLE), true);
    /* Инициализация сама не вызывает callback */
    ck_assert_int_eq(test_ctx.callback_calls, 0);
    /* Явный init-событие */
    fsm_process_event(&test_fsm, TEST_STATE_IDLE);
    ck_assert_int_eq(test_ctx.callback_calls, 1);
    ck_assert_int_eq(test_ctx.last_state, TEST_STATE_IDLE);
}
END_TEST

/* ===============================================
 * Тесты функции fsm_destroy()
 * =============================================== */

START_TEST(test_fsm_destroy_valid) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    /* Функция destroy в текущей реализации не делает ничего, 
       но должна корректно обрабатывать валидный указатель */
    fsm_destroy(&test_fsm);
    // Проверка завершается успешно, если не было падения
}
END_TEST

START_TEST(test_fsm_destroy_null) {
    /* Функция должна корректно обрабатывать NULL указатель */
    fsm_destroy(NULL);
    // Проверка завершается успешно, если не было падения
}
END_TEST

/* ===============================================
 * Тесты функции fsm_process_event()
 * =============================================== */

START_TEST(test_fsm_process_event_valid_transition) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_START);
    
    ck_assert_int_eq(result, true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    ck_assert_int_eq(test_ctx.callback_calls, 2);  // on_exit_idle + on_enter_active
    ck_assert_int_eq(test_ctx.counter, 1);
}
END_TEST

START_TEST(test_fsm_process_event_invalid_transition) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_STOP);
    
    ck_assert_int_eq(result, false);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);  // Состояние не изменилось
    ck_assert_int_eq(test_ctx.callback_calls, 0);
}
END_TEST

START_TEST(test_fsm_process_event_null_fsm) {
    bool result = fsm_process_event(NULL, TEST_EVENT_START);
    
    ck_assert_int_eq(result, false);
}
END_TEST

START_TEST(test_fsm_process_event_sequence) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    /* Последовательность: IDLE -> ACTIVE -> PAUSED -> ACTIVE -> IDLE */
    ck_assert_int_eq(fsm_process_event(&test_fsm, TEST_EVENT_START), true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    
    ck_assert_int_eq(fsm_process_event(&test_fsm, TEST_EVENT_PAUSE), true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_PAUSED);
    
    ck_assert_int_eq(fsm_process_event(&test_fsm, TEST_EVENT_RESUME), true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    
    ck_assert_int_eq(fsm_process_event(&test_fsm, TEST_EVENT_STOP), true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
    
    ck_assert_int_eq(test_ctx.counter, 2);  // Два входа в ACTIVE
}
END_TEST

START_TEST(test_fsm_process_event_callback_null) {
    /* Тест переходов с NULL callback'ами */
    fsm_transition_t null_callback_transitions[] = {
        {TEST_STATE_IDLE, TEST_EVENT_START, TEST_STATE_ACTIVE, NULL, NULL}
    };
    
    fsm_init(&test_fsm, &test_ctx, null_callback_transitions, 1, TEST_STATE_IDLE);
    
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_START);
    
    ck_assert_int_eq(result, true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    ck_assert_int_eq(test_ctx.callback_calls, 0);  // Callback'и не вызывались
}
END_TEST

/* ===============================================
 * Тесты функции fsm_update()
 * =============================================== */

START_TEST(test_fsm_update_auto_transition) {
    fsm_init(&test_fsm, &test_ctx, auto_transitions, 
             sizeof(auto_transitions)/sizeof(auto_transitions[0]), 
             TEST_STATE_IDLE);
    
    /* Переходим в ACTIVE */
    fsm_process_event(&test_fsm, TEST_EVENT_START);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    
    /* Автоматический переход ACTIVE -> PAUSED */
    fsm_update(&test_fsm);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_PAUSED);
    
    /* Автоматический переход PAUSED -> IDLE */
    fsm_update(&test_fsm);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
    
    /* Больше автоматических переходов нет */
    fsm_update(&test_fsm);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
}
END_TEST

START_TEST(test_fsm_update_no_auto_transition) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    fsm_state_t initial_state = test_fsm.current;
    
    fsm_update(&test_fsm);
    
    ck_assert_int_eq(test_fsm.current, initial_state);  // Состояние не изменилось
}
END_TEST

START_TEST(test_fsm_update_null_fsm) {
    fsm_update(NULL);
    // Проверка завершается успешно, если не было падения
}
END_TEST

/* ===============================================
 * Тесты покрытия edge cases
 * =============================================== */

START_TEST(test_fsm_edge_case_self_transition) {
    fsm_transition_t self_transitions[] = {
        {TEST_STATE_IDLE, TEST_EVENT_START, TEST_STATE_IDLE, on_exit_idle, on_enter_idle}
    };
    
    fsm_init(&test_fsm, &test_ctx, self_transitions, 1, TEST_STATE_IDLE);
    
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_START);
    
    ck_assert_int_eq(result, true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
    ck_assert_int_eq(test_ctx.callback_calls, 2);  // on_exit + on_enter
}
END_TEST

START_TEST(test_fsm_edge_case_multiple_transitions_same_event) {
    /* Первый найденный переход должен выполниться */
    fsm_transition_t multi_transitions[] = {
        {TEST_STATE_IDLE, TEST_EVENT_START, TEST_STATE_ACTIVE, NULL, on_enter_active},
        {TEST_STATE_IDLE, TEST_EVENT_START, TEST_STATE_PAUSED, NULL, on_enter_paused}  // Не должен выполниться
    };
    
    fsm_init(&test_fsm, &test_ctx, multi_transitions, 2, TEST_STATE_IDLE);
    
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_START);
    
    ck_assert_int_eq(result, true);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    ck_assert_int_eq(test_ctx.last_state, TEST_STATE_ACTIVE);
}
END_TEST

START_TEST(test_fsm_edge_case_error_recovery) {
    fsm_init(&test_fsm, &test_ctx, error_transitions, 
             sizeof(error_transitions)/sizeof(error_transitions[0]), 
             TEST_STATE_IDLE);
    
    /* Переходим в активное состояние */
    fsm_process_event(&test_fsm, TEST_EVENT_START);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    
    /* Генерируем ошибку */
    fsm_process_event(&test_fsm, TEST_EVENT_ERROR);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ERROR);
    
    /* Автоматическое восстановление */
    fsm_update(&test_fsm);
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
}
END_TEST

/* ===============================================
*  Тесты защиты от рекурсивного вызова
*  =============================================== */

START_TEST(test_fsm_recursion_protection) {
    test_ctx.fsm_ref = &test_fsm;
    test_ctx.callback_calls = 0;  // ← Убедиться что счётчик обнулен
    
    ck_assert_int_eq(fsm_init(&test_fsm, &test_ctx, recursion_transitions, 2, TEST_STATE_IDLE), true);
    
    // При входе в ACTIVE вызывается on_enter_with_event_call → вызывает TEST_EVENT_STOP
    bool result = fsm_process_event(&test_fsm, TEST_EVENT_START);
    
    // Основной переход выполнен успешно
    ck_assert_int_eq(result, true);
    
    // FSM остаётся в ACTIVE (рекурсивный вызов был заблокирован)
    ck_assert_int_eq(test_fsm.current, TEST_STATE_ACTIVE);
    
    // on_enter_with_event_call был вызван один раз
    ck_assert_int_eq(test_ctx.callback_calls, 1);
}
END_TEST


/* ===============================================
 * Тесты производительности и стресс-тесты
 * =============================================== */

START_TEST(test_fsm_stress_many_transitions) {
    fsm_init(&test_fsm, &test_ctx, basic_transitions, 
             sizeof(basic_transitions)/sizeof(basic_transitions[0]), 
             TEST_STATE_IDLE);
    
    /* Выполняем много переходов */
    for (int i = 0; i < 1000; i++) {
        fsm_process_event(&test_fsm, TEST_EVENT_START);
        fsm_process_event(&test_fsm, TEST_EVENT_PAUSE);
        fsm_process_event(&test_fsm, TEST_EVENT_RESUME);
        fsm_process_event(&test_fsm, TEST_EVENT_STOP);
    }
    
    ck_assert_int_eq(test_fsm.current, TEST_STATE_IDLE);
    ck_assert_int_eq(test_ctx.counter, 2000);  // 2 входа в ACTIVE за итерацию
}
END_TEST

/* ===============================================
 * Создание тестовых наборов
 * =============================================== */

Suite *fsm_init_suite(void) {
    Suite *s = suite_create("FSM Init");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_init_success);
    tcase_add_test(tc_core, test_fsm_init_null_fsm);
    tcase_add_test(tc_core, test_fsm_init_null_transitions);
    tcase_add_test(tc_core, test_fsm_init_zero_count);
    tcase_add_test(tc_core, test_fsm_init_callback_execution);
    suite_add_tcase(s, tc_core);
    return s;
}

Suite *fsm_destroy_suite(void) {
    Suite *s = suite_create("FSM Destroy");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_destroy_valid);
    tcase_add_test(tc_core, test_fsm_destroy_null);
    suite_add_tcase(s, tc_core);
    return s;
}

Suite *fsm_process_event_suite(void) {
    Suite *s = suite_create("FSM Process Event");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_process_event_valid_transition);
    tcase_add_test(tc_core, test_fsm_process_event_invalid_transition);
    tcase_add_test(tc_core, test_fsm_process_event_null_fsm);
    tcase_add_test(tc_core, test_fsm_process_event_sequence);
    tcase_add_test(tc_core, test_fsm_process_event_callback_null);
    suite_add_tcase(s, tc_core);
    return s;
}

Suite *fsm_update_suite(void) {
    Suite *s = suite_create("FSM Update");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_update_auto_transition);
    tcase_add_test(tc_core, test_fsm_update_no_auto_transition);
    tcase_add_test(tc_core, test_fsm_update_null_fsm);
    suite_add_tcase(s, tc_core);
    return s;
}

Suite *fsm_edge_cases_suite(void) {
    Suite *s = suite_create("FSM Edge Cases");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_edge_case_self_transition);
    tcase_add_test(tc_core, test_fsm_edge_case_multiple_transitions_same_event);
    tcase_add_test(tc_core, test_fsm_edge_case_error_recovery);
    tcase_add_test(tc_core, test_fsm_recursion_protection);
    suite_add_tcase(s, tc_core);
    return s;
}

Suite *fsm_stress_suite(void) {
    Suite *s = suite_create("FSM Stress Tests");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_fsm_stress_many_transitions);
    suite_add_tcase(s, tc_core);
    return s;
}

/* ===============================================
 * Главная функция тестирования
 * =============================================== */

int main(void) {
    int number_failed = 0;
    
    /* Создаем и запускаем все тестовые наборы */
    Suite *suites[] = {
        fsm_init_suite(),
        fsm_destroy_suite(),
        fsm_process_event_suite(),
        fsm_update_suite(),
        fsm_edge_cases_suite(),
        fsm_stress_suite()
    };
    
    const int num_suites = sizeof(suites) / sizeof(suites[0]);
    
    for (int i = 0; i < num_suites; i++) {
        SRunner *sr = srunner_create(suites[i]);
        srunner_set_fork_status(sr, CK_NOFORK);
        srunner_run_all(sr, CK_NORMAL);
        number_failed += srunner_ntests_failed(sr);
        srunner_free(sr);
    }
    
    printf("\n=== Результаты тестирования FSM ===\n");
    printf("Общее количество неудачных тестов: %d\n", number_failed);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}