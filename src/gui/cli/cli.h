/**
 * @file cli.h
 * @brief Публичный интерфейс CLI-реализации View для BrickGame (через ncurses).
 *
 * Содержит объявление экспортируемого объекта `cli_view`, реализующего
 * универсальный интерфейс отображения @ref ViewInterface. Предоставляет
 * текстовый интерфейс на основе библиотеки ncurses.
 *
 * Пример использования:
 * @code
 * ViewHandle_t view = cli_view.init(10, 20, 25);
 * cli_view.configure_zone(view, "field", 1, 1, 10, 20);
 * // ... отрисовка, ввод, обновление
 * cli_view.shutdown(view);
 * @endcode
 *
 * @note Для компиляции требуется подключение библиотеки ncurses: -lncurses.
 * @note Функции инициализируют и управляют состоянием ncurses самостоятельно
 *       (включая initscr(), endwin() и настройку режимов).
 * @note Не используйте ncurses напрямую при работе с этим интерфейсом —
 *       это может привести к конфликту состояний.
 *
 * @defgroup Cli_view Реализация CLI-интерфейса
 * @ingroup View
 *
 * @author provemet
 * @copyright (c) December 2024
 * @version 1.0
 */

#ifndef CLI_H
#define CLI_H

#include "../common/view.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Экземпляр интерфейса отображения для CLI-бэкенда на базе ncurses.
 *
 * Глобальный объект, реализующий абстрактный интерфейс ViewInterface.
 * Предоставляет текстовый пользовательский интерфейс через библиотеку ncurses.
 * Все функции являются потоконебезопасными и должны вызываться
 * из одного потока — основного цикла приложения.
 *
 * Пример использования:
 * @code
 * ViewHandle_t view = cli_view.init(10, 20, 25);
 * if (!view) {
 *     fprintf(stderr, "Не удалось инициализировать CLI-интерфейс\n");
 *     return -1;
 * }
 *
 * cli_view.configure_zone(view, "field", 1, 1, 10, 20);
 * // ... основной цикл: draw_element, render, poll_input
 * cli_view.shutdown(view);
 * @endcode
 *
 * @note Объект является const и должен использоваться только для чтения.
 * @note Не модифицируйте его поля — это приведёт к неопределённому поведению.
 * @note Реализация управляет внутренним состоянием ncurses — не вызывайте
 *       initscr(), endwin() и другие функции ncurses напрямую.
 */
extern const ViewInterface cli_view;

#ifdef __cplusplus
}
#endif

#endif // CLI_H

/** @} */