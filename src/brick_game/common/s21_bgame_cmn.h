/**
 * @file s21_bgame_cmn.h
 * @brief Общие утилиты для всех игр BrickGame
 *
 * Этот заголовочный файл предоставляет переиспользуемые функции и утилиты,
 * которые используются совместно всеми играми (Tetris, Snake, Racing и т.д.).
 * Эти функции обрабатывают:
 * - Управление памятью для игрового поля и превью
 * - Сохранение и загрузку рекордов (на диск)
 * - Инициализацию и очистку структуры GameInfo
 * - Валидацию вводимых данных
 *
 * Все игры должны использовать эти функции вместо реализации собственной
 * логики управления памятью и сохранения рекордов.
 *
 * @author provemet
 * @version 1.0
 * @date 2025-12-16
 */

#ifndef BRICKGAME_COMMON_H
#define BRICKGAME_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "s21_bgame.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Выделить память для игрового поля (10x20)
 *
 * Выделяет 2D массив размером 20x10 целых чисел для игрового поля.
 * Каждый элемент инициализируется значением 0 (пусто).
 *
 * Выделенная память готова к прямому использованию:
 * @code
 * int **field = brickgame_allocate_field();
 * field[0][0] = 1;  // Установить левый верхний блок как заполненный
 * brickgame_free_field(field);
 * @endcode
 *
 * @return Указатель на новое выделенное поле (массив 20x10), или NULL при ошибке
 *
 * @note Вызывающая сторона ответственна за освобождение этой памяти
 *       с помощью brickgame_free_field() когда она больше не нужна.
 *
 * @see brickgame_free_field()
 */
int** brickgame_allocate_field(void);

/**
 * @brief Освободить память игрового поля
 *
 * Освобождает память, ранее выделенную функцией brickgame_allocate_field().
 * Безопасна при вызове с NULL указателем.
 *
 * @param field Указатель на поле для освобождения
 *
 * @note После вызова этой функции указатель field становится недействительным
 *       и не должен больше использоваться.
 *
 * @see brickgame_allocate_field()
 */
void brickgame_free_field(int **field);

/**
 * @brief Очистить все ячейки поля на 0
 *
 * Устанавливает все ячейки в поле в значение 0 (пусто).
 * Не освобождает память.
 *
 * @param field Указатель на поле для очистки
 *
 * @note Безопасна при вызове с NULL (ничего не делает).
 *
 * @see brickgame_allocate_field()
 */
void brickgame_clear_field(int **field);

/**
 * @brief Выделить память для превью следующей фигуры (4x4)
 *
 * Выделяет 2D массив размером 4x4 целых чисел для превью следующей фигуры.
 * Каждый элемент инициализируется значением 0 (пусто).
 *
 * Используется для отображения того, какая фигура/предмет появится следующей в игре.
 *
 * @code
 * int **next = brickgame_allocate_next();
 * next[0][0] = 1;  // Отметить первую ячейку как часть следующей фигуры
 * brickgame_free_next(next);
 * @endcode
 *
 * @return Указатель на новое выделенное превью (массив 4x4), или NULL при ошибке
 *
 * @note Вызывающая сторона ответственна за освобождение этой памяти
 *       с помощью brickgame_free_next() когда она больше не нужна.
 *
 * @see brickgame_free_next()
 */
int** brickgame_allocate_next(void);

/**
 * @brief Освободить память превью следующей фигуры
 *
 * Освобождает память, ранее выделенную функцией brickgame_allocate_next().
 * Безопасна при вызове с NULL указателем.
 *
 * @param next Указатель на массив превью для освобождения
 *
 * @note После вызова этой функции указатель next становится недействительным
 *       и не должен больше использоваться.
 *
 * @see brickgame_allocate_next()
 */
void brickgame_free_next(int **next);

/**
 * @brief Очистить все ячейки превью на 0
 *
 * Устанавливает все ячейки в превью на значение 0 (пусто).
 * Не освобождает память.
 *
 * @param next Указатель на превью для очистки
 *
 * @note Безопасна при вызове с NULL (ничего не делает).
 *
 * @see brickgame_allocate_next()
 */
void brickgame_clear_next(int **next);

/**
 * @brief Загрузить рекордный счёт игры с диска
 *
 * Читает рекордный счёт из файла на диске, сохранённого ранее.
 * Расположение файла: ~/.brickgame/<имя_игры>.score
 *
 * Если файл не существует (первый запуск), возвращает 0.
 * Если чтение не удаётся, возвращает 0 и выводит ошибку в stderr.
 *
 * Пример использования:
 * @code
 * int high_score = brickgame_load_high_score("tetris");
 * printf("Рекорд: %d\n", high_score);
 * @endcode
 *
 * @param game_name Имя игры (например, "tetris", "snake")
 *                  Используется для построения имени файла
 *
 * @return Значение рекорда (0 или больше), или 0 если файл не существует
 *
 * @note Если game_name равен NULL, возвращает 0
 * @note Директория .brickgame создаётся автоматически, если не существует
 *
 * @see brickgame_save_high_score()
 */
int brickgame_load_high_score(const char *game_name);

/**
 * @brief Сохранить рекордный счёт игры на диск
 *
 * Записывает рекордный счёт в файл на диск для сохранения между сеансами.
 * Расположение файла: ~/.brickgame/<имя_игры>.score
 *
 * Перезаписывает любой существующий файл счёта.
 * Создаёт директорию .brickgame, если она не существует.
 *
 * Пример использования:
 * @code
 * bool success = brickgame_save_high_score("tetris", 12345);
 * if (success) {
 *     printf("Счёт сохранён успешно\n");
 * } else {
 *     printf("Ошибка при сохранении счёта\n");
 * }
 * @endcode
 *
 * @param game_name Имя игры (например, "tetris", "snake")
 *                  Используется для построения имени файла
 * @param score     Счёт для сохранения (должен быть неотрицательным)
 *
 * @return true при успехе, false при ошибке
 *
 * @note Если game_name равен NULL или score < 0, возвращает false
 * @note Директория .brickgame создаётся автоматически, если не существует
 *
 * @see brickgame_load_high_score()
 */
bool brickgame_save_high_score(const char *game_name, int score);

/**
 * @brief Создать и инициализировать структуру GameInfo_t
 *
 * Выделяет всю необходимую память для структуры GameInfo_t:
 * - Выделяет поле (10x20)
 * - Выделяет превью (4x4)
 * - Инициализирует все поля значениями по умолчанию:
 *   - score = 0
 *   - high_score = 0
 *   - level = 1
 *   - speed = 800
 *   - pause = 0
 *
 * Пример использования:
 * @code
 * GameInfo_t info = brickgame_create_game_info();
 * if (info.field != NULL && info.next != NULL) {
 *     // Структура GameInfo готова к использованию
 *     info.score = 100;
 *     // ...
 * } else {
 *     // Ошибка выделения памяти
 * }
 * brickgame_destroy_game_info(&info);
 * @endcode
 *
 * @return Инициализированная структура GameInfo_t
 *
 * @note Если выделение памяти не удаётся, поля field и next будут NULL.
 *       Проверьте эти указатели перед использованием структуры.
 *
 * @see brickgame_destroy_game_info()
 */
GameInfo_t brickgame_create_game_info(void);

/**
 * @brief Освободить структуру GameInfo_t
 *
 * Освобождает всю память, ранее выделенную функцией brickgame_create_game_info().
 * Устанавливает указатели field и next в NULL.
 *
 * Безопасна при вызове с NULL указателем.
 *
 * Пример использования:
 * @code
 * GameInfo_t info = brickgame_create_game_info();
 * // ... использование info ...
 * brickgame_destroy_game_info(&info);
 * @endcode
 *
 * @param info Указатель на структуру GameInfo_t для освобождения
 *
 * @note После вызова этой функции структура info обнулится полностью
 *
 * @see brickgame_create_game_info()
 */
void brickgame_destroy_game_info(GameInfo_t *info);

/**
 * @brief Проверить корректность значения UserAction
 *
 * Проверяет, является ли данное значение допустимым значением перечисления UserAction_t.
 *
 * @param action Значение действия для проверки
 *
 * @return true если действие допустимо (от Start до Action), false иначе
 *
 * Пример использования:
 * @code
 * if (brickgame_is_valid_action(user_action)) {
 *     userInput(user_action, false);
 * } else {
 *     printf("Недопустимое действие: %d\n", user_action);
 * }
 * @endcode
 */
bool brickgame_is_valid_action(UserAction_t action);

/**
 * @brief Проверить корректность указателя и содержимого поля
 *
 * Проверяет следующее:
 * - Указатель field не является NULL
 * - Поле правильно выделено (20 строк)
 * - Каждый указатель строки не является NULL
 * - Каждая ячейка содержит допустимое значение (0 или 1)
 *
 * @param field Указатель на поле для проверки
 *
 * @return true если поле корректно, false иначе
 *
 * @note Эта функция не проверяет ширину строк (предполагает 10 столбцов)
 *
 * Пример использования:
 * @code
 * int **field = brickgame_allocate_field();
 * if (brickgame_is_valid_field(field)) {
 *     // Безопасно использовать поле
 * }
 * @endcode
 *
 * @see brickgame_allocate_field()
 */
bool brickgame_is_valid_field(int **field);

/**
 * @brief Проверить корректность структуры GameInfo_t
 *
 * Проверяет следующее:
 * - Указатели (field, next) не являются NULL
 * - field корректно (см. brickgame_is_valid_field())
 * - next корректно (массив 4x4 с допустимыми значениями)
 * - score и high_score неотрицательны
 * - level в диапазоне [1, 10]
 * - speed неотрицательна
 * - pause равна 0 или 1
 *
 * @param info Структура GameInfo_t для проверки
 *
 * @return true если все поля корректны, false иначе
 *
 * Пример использования:
 * @code
 * GameInfo_t state = updateCurrentState();
 * if (brickgame_is_valid_game_info(&state)) {
 *     // Безопасно отрисовать состояние
 * } else {
 *     // Некорректное состояние, обработать ошибку
 * }
 * @endcode
 */
bool brickgame_is_valid_game_info(const GameInfo_t *info);

#ifdef __cplusplus
}
#endif

#endif /* BRICKGAME_COMMON_H */