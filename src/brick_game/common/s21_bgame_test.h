/**
 * @file s21_bgame_test.h
 * @brief Приватный интерфейс для unit-тестов модуля s21_bgame
 *
 * Содержит функции сброса внутреннего состояния фреймворка, необходимые
 * исключительно для изоляции и повторяемости unit-тестов. Не предназначен
 * для использования в релизной сборке.
 *
 * @warning Включайте этот заголовок только в тестовых исходных файлах.
 *          Должен использоваться совместно с флагом -DTEST_ENV.
 *
 * @note Эти функции не являются потокобезопасными и могут нарушить
 *       согласованность состояния, если вызываются во время работы игры.
 *
 * Пример использования:
 * @code
 * #define TEST_ENV
 * #include "s21_bgame_test.h"
 *
 * void test_registration_isolated() {
 *     bg_reset_registry_for_testing();  // Чистое состояние
 *     bg_register_game(tetris_interface);
 *     assert(bg_get_registered_count() == 1);
 *     bg_reset_registry_for_testing();  // Сброс после теста
 * }
 * @endcode
 *
 * @see s21_bgame.h
 * @author provemet
 * @date 2025
 * @version 1.0
 */
#ifndef S21_BGAME_TEST_H_
#define S21_BGAME_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Сбрасывает контекст текущей активной игры (используется только в тестах)
 *
 * Полностью обнуляет глобальный контекст активной игры: устанавливает указатели
 * интерфейса и экземпляра в NULL. Используется для изоляции unit-тестов,
 * чтобы исключить влияние состояния предыдущих тестов.
 *
 * @warning Предназначена исключительно для использования в unit-тестах
 *          при сборке с флагом -DTEST_ENV. Не включайте в релизную сборку.
 *
 * @note Функция безопасна для повторного вызова и не вызывает побочных эффектов,
 *       даже если контекст уже сброшен или не был инициализирован.
 *
 * @par Пример использования:
 * @code
 * void test_switch_to_tetris() {
 *     bg_reset_current_for_testing();  // Чистое состояние
 *     assert(bg_get_current_instance() == NULL);
 *     
 *     bg_switch_game(GAME_TETRIS);
 *     assert(bg_get_current_instance() != NULL);
 *     
 *     bg_reset_current_for_testing();  // Очистка после теста
 * }
 * @endcode
 *
 * @see bg_reset_registry_for_testing(), bg_switch_game()
 */
void bg_reset_current_for_testing(void);

/**
 * @brief Сбрасывает реестр игр в начальное состояние (используется только в тестах)
 *
 * Полностью очищает глобальный реестр зарегистрированных игр:
 * 1. Выгружает все динамически загруженные плагины с помощью bg_unload_all_plugins()
 * 2. Освобождает память, выделенную под массив реестра
 * 3. Сбрасывает все счётчики и указатели реестра в нулевые значения
 *
 * @warning Эта функция предназначена исключительно для использования в unit-тестах
 *          при сборке с флагом -DTEST_ENV. Не используйте в рабочем коде.
 *
 * @note После вызова функции:
 *       - Все зарегистрированные игры (включая плагины) будут удалены
 *       - Реестр будет находиться в состоянии "до первой инициализации"
 *       - Последующие вызовы bg_register_game() будут работать как при первом запуске
 *
 * @par Пример использования в тестах:
 * @code
 * void test_game_registration() {
 *     // Подготовка
 *     bg_reset_registry_for_testing();
 *     
 *     // Регистрация тестовой игры
 *     GameInterface_t test_game = get_test_interface();
 *     bg_register_game(test_game);
 *     
 *     // Проверка
 *     assert(bg_get_registered_count() == 1);
 *     
 *     // Очистка после теста
 *     bg_reset_registry_for_testing();
 * }
 * @endcode
 *
 * @see bg_unload_all_plugins(), bg_reset_current_for_testing()
 */
void bg_reset_registry_for_testing(void);

#ifdef __cplusplus
}
#endif

#endif  // S21_BGAME_TEST_H_
