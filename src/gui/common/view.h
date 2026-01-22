/**
 * @file view.h
 * @brief Публичный API для универсального модуля отображения BrickGame
 *
 * Этот заголовочный файл определяет абстрактный интерфейс (View) для реализации любых видов отображения
 * (CLI, GUI, Web и др.) в проекте BrickGame. View полностью изолирован от игровой логики (модели).
 * Контроллер связывает модель и представление через этот API.
 *
 * Основные идеи:
 * - ViewHandle_t — абстрактный указатель на внутренний контекст интерфейса.
 * - Все функции работают только с этим handle, обеспечивая модульность.
 * - Универсальные структуры для текста, чисел, матриц.
 * - Ввод описывается через InputEvent_t.
 * - Поддержка версионирования и обработки ошибок.
 *
 * @author provemet
 * @date December 2024
 * @defgroup View Публичный интерфейс библиотек отображения игр BrickGame
 * @ingroup BrickGame
 * @{
 */

#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef ViewHandle_t
 * @brief Абстрактный указатель на внутренний контекст View-модуля.
 *
 * Контроллер получает этот handle при инициализации интерфейса и передаёт во все функции View.
 * Содержимое скрыто от пользователя — это обеспечивает независимость и безопасность.
 */
typedef void *ViewHandle_t;

/**
 * @enum ViewResult_t
 * @brief Результат выполнения операций View.
 */
typedef enum {
    VIEW_OK,              ///< Операция успешна
    VIEW_ERROR,           ///< Общая ошибка
    VIEW_INVALID_ID,      ///< Недопустимый element_id
    VIEW_BAD_DATA,        ///< Некорректные данные (например, NULL data при matrix)
    VIEW_NOT_INITIALIZED, ///< View не инициализирован
    VIEW_NO_EVENT         ///< Событие ввода отсутствует (для poll_input)
} ViewResult_t;

/**
 * @struct InputEvent_t
 * @brief Описывает событие ввода пользователя.
 *
 * @var InputEvent_t::key_code
 *     Код клавиши (например, KEY_LEFT). Если событие отсутствует, key_code == 0.
 * @var InputEvent_t::key_state
 *     - 0: однократное нажатие
 *     - 1: удержание
 *
 * @note Если нет события, возвращается {.key_code = 0, .result = VIEW_NO_EVENT}
 */
typedef struct {
    int key_code;
    int key_state;
} InputEvent_t;

/**
 * @enum ElementType_t
 * @brief Тип содержимого для отрисовки.
 */
typedef enum {
    ELEMENT_TEXT,   ///< const char* (поддерживает '\n')
    ELEMENT_NUMBER, ///< int
    ELEMENT_MATRIX  ///< двумерный массив int
} ElementType_t;

/**
 * @struct ElementData_t
 * @brief Универсальный контейнер для передачи данных.
 *
 * @note Память, на которую указывает content.matrix.data, не копируется.
 *       Данные должны быть валидны до вызова render().
 * @note Массив хранится в row-major порядке: элемент (x,y) → индекс = y * width + x.
 * @note Максимальная длина строки text — 512 байт (включая '\0').
 */
typedef struct ElementData_t {
    ElementType_t type;
    union {
        const char *text;      ///< для ELEMENT_TEXT
        int         number;    ///< для ELEMENT_NUMBER
        struct {               ///< для ELEMENT_MATRIX
            const int *data;   ///< указатель на данные (row-major)
            int        width;  ///< ширина матрицы
            int        height; ///< высота матрицы
        } matrix;
    } content;
} ElementData_t;

/**
 * @struct ColorPair_t
 * @brief Цветовая пара: передний план и фон.
 *
 * Значения зависят от бэкенда (например, 0-255 или константы ncurses).
 */
typedef struct {
    int foreground; ///< цвет текста/символа
    int background; ///< цвет фона
} ColorPair_t;

/**
 * @brief Главный интерфейс View (аналог vtable).
 *
 * Реализация должна экспортировать экземпляр этой структуры.
 *
 * Пример:
 * @code
 * extern const ViewInterface tetris_view;
 * ViewHandle_t view = tetris_view.init(10, 20, 30);
 * @endcode
 */
typedef struct ViewInterface {
    int version; ///< Версия интерфейса (текущая: 1)

    /**
     * @brief Инициализация движка отображения.
     * @param width  Ширина поля в символах
     * @param height Высота поля в символах
     * @param fps    Частота обновления кадров
     * @return Контекст или NULL при ошибке
     */
    ViewHandle_t (*init)(int width, int height, int fps);

    /**
     * @brief Настраивает зону вывода.
     * @param handle     Контекст
     * @param element_id Имя зоны (макс. 31 символ, без '\0')
     * @param x, y       Позиция в символах
     * @param max_width  Макс. ширина (в символах)
     * @param max_height Макс. высота (в символах)
     * @return VIEW_OK при успехе
     *
     * @note Если зона уже существует — перезаписывается.
     */
    ViewResult_t (*configure_zone)(ViewHandle_t handle,
                                   const char   *element_id,
                                   int           x,
                                   int           y,
                                   int           max_width,
                                   int           max_height);

    /**
     * @brief Отрисовывает элемент в буфер.
     * @param handle     Контекст
     * @param element_id Идентификатор зоны
     * @param data       Данные для отрисовки
     * @return VIEW_OK при успехе
     *
     * @note Данные не копируются. Убедитесь, что память валидна до render().
     * @note type должен соответствовать заполненному полю content.
     */
    ViewResult_t (*draw_element)(ViewHandle_t          handle,
                                 const char            *element_id,
                                 const ElementData_t   *data);

    /**
     * @brief Выводит буфер на экран.
     * @param handle Контекст
     * @return VIEW_OK при успехе
     */
    ViewResult_t (*render)(ViewHandle_t handle);

    /**
     * @brief Читает событие ввода.
     * @param handle Контекст
     * @param event  Указатель, куда записать событие
     * @return VIEW_OK при наличии события, VIEW_NO_EVENT — если нет
     *
     * @note Всегда проверяйте возвращаемое значение.
     */
    ViewResult_t (*poll_input)(ViewHandle_t handle, InputEvent_t *event);

    /**
     * @brief Завершает работу и освобождает ресурсы.
     * @param handle Контекст
     * @return VIEW_OK или VIEW_ERROR
     *
     * @note handle становится недействительным после вызова.
     */
    ViewResult_t (*shutdown)(ViewHandle_t handle);
} ViewInterface;

/* Текущая версия API */
#define VIEW_INTERFACE_VERSION 1

#ifdef __cplusplus
}
#endif

#endif // VIEW_H

/** @} */  // end of View module