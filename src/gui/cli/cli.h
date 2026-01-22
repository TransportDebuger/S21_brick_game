/**
 * @file cli.h
 * @brief Экземпляр CLI-интерфейса для BrickGame (ncurses)
 *
 * Здесь объявляется экспортируемый объект `cli_view`, реализующий
 * абстрактный API из @ref View.
 * 
 * @note Требует подключения библиотеки ncurses (-lncurses).
 *       Функции инициализируют и управляют ncurses самостоятельно.
 * 
 * @defgroup CliView Реализация библиотеки Cli интерфейса
 * @ingroup View
 * 
 * @author provemet
 * @date December 2024
 * 
 * @{
 */

#ifndef CLI_H
#define CLI_H

#include "../common/view.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Экземпляр CLI-интерфейса для BrickGame.
 *
 * Этот объект содержит указатели на функции, реализующие
 * CLI (ncurses) вариант представления.
 *
 * Пример использования:
 * @code
 * ViewHandle_t view = cli_view.init(10, 20, 30);
 * cli_view.configure_zone(view, "field", 1, 1, 10, 20);
 * // ...
 * cli_view.shutdown(view);
 * @endcode
 */
extern const ViewInterface cli_view;

#ifdef __cplusplus
}
#endif

#endif // CLI_H

/** @} */