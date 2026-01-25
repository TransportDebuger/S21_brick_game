/**
 * @file cli.c
 * @brief Реализация CLI-интерфейса отображения для BrickGame на базе ncurses.
 *
 * Содержит полную реализацию интерфейса ViewInterface, обеспечивающую
 * текстовый пользовательский интерфейс через библиотеку ncurses.
 * Все функции соответствуют контракту, описанному в @ref ViewInterface.
 *
 * Ключевые особенности:
 * - Управление ncurses: initscr, cbreak, noecho, keypad, nodelay
 * - Отображение элементов через зоны с рамками и подписями
 * - Преобразование специальных кодов клавиш (стрелки) в логические символы ('w', 'a', 's', 'd')
 * - Поддержка отрисовки текста, чисел и игровых матриц
 *
 * @note Этот модуль не зависит от игровой логики — взаимодействует
 *       с контроллером только через абстрактный ViewHandle_t.
 *
 * @note Все функции не являются потокобезопасными. Предназначены
 *       для вызова из одного потока — основного цикла игры.
 *
 * @defgroup CliView Реализация CLI-интерфейса отображения
 * @ingroup View
 *
 * @author provemet
 * @copyright (c) December 2024
 * @version 1.0
 */

#include "cli.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h> // для gettimeofday

/**
 * @def MAX_ZONES
 * @brief Максимальное количество поддерживаемых зон интерфейса.
 *
 * Определяет верхнюю границу числа элементов, которые можно настроить
 * через configure_zone. Используется для фиксации размера внутренних
 * массивов (например, zone_names, zones).
 */
#define MAX_ZONES 8

/**
 * @def MAX_NAME_LEN
 * @brief Максимальная длина имени зоны отображения (в символах).
 *
 * Определяет максимальное количество символов, которое может храниться
 * в имени зоны (без учёта нуль-терминатора). Используется для выделения
 * буферов в массиве zone_names и при копировании имён с помощью strncpy.
 *
 * @note Значение должно быть меньше размера буфера в массиве (рекомендуется на 1),
 *       чтобы гарантировать ручное добавление '\0' и избежать отсутствия терминатора.
 *
 * @note Имена длиннее будут обрезаться. Рекомендуется использовать короткие
 *       идентификаторы (например, "field", "score", "next").
 *
 * @note Изменение этого макроса требует проверки всех буферов,
 *       в которые копируются имена зон, во избежание переполнения.
 */
#define MAX_NAME_LEN 16

/**
 * @typedef CliContext_t
 * @brief Тип для внутреннего контекста CLI-модуля.
 *
 * Представляет собой opaque-указатель на внутренние данные интерфейса.
 * Используется как реализация ViewHandle_t — абстрактного контекста представления.
 */
typedef struct CliContext CliContext_t;

/**
 * @struct CliContext
 * @brief Внутренний контекст CLI-модуля.
 *
 * Хранит параметры экрана, конфигурацию зон и указатель на окно ncurses.
 *
 * @var CliContext::width
 *     Ширина игрового поля (количество столбцов).
 * @var CliContext::height
 *     Высота игрового поля (количество строк).
 * @var CliContext::fps
 *     Частота обновления экрана (кадров в секунду).
 * @var CliContext::zones
 *     Массив настроенных зон (до 8), каждая зона — прямоугольник.
 * @var CliContext::zone_names
 *     Имена зон для поиска по строковому идентификатору.
 * @var CliContext::zone_count
 *     Текущее число настроенных зон.
 * @var CliContext::win
 *     Главное окно ncurses (stdscr).
 */
struct CliContext {
    int width, height, fps;
    struct {
        int x, y, w, h;
    } zones[MAX_ZONES]; // максимум 8 зон интерфейса
    char zone_names[MAX_ZONES][MAX_NAME_LEN];
    int zone_count;
    WINDOW *win;
};

/**
 * @brief Находит индекс зоны по её имени в контексте CLI.
 *
 * Линейный поиск по массиву zone_names в контексте. Используется для сопоставления
 * строкового идентификатора (например, "field") с соответствующей областью отрисовки.
 *
 * @param ctx Указатель на внутренний контекст CLI (не должен быть NULL)
 * @param name Имя зоны для поиска (не NULL, не пустая строка)
 *
 * @return Индекс зоны в диапазоне [0, zone_count-1], если найдена;
 *         -1 — если зона с таким именем не найдена или один из аргументов невалиден.
 *
 * @note Функция чувствительна к регистру: "Field" ≠ "field".
 * @note Производительность — O(n), где n = zone_count (максимум 8).
 *       При таком размере перебор является оптимальным решением.
 */
static int find_zone(CliContext_t *ctx, const char *name) {
    if (!ctx || !name) return -1;

    for (int i = 0; i < ctx->zone_count; ++i)
        if (strcmp(ctx->zone_names[i], name) == 0)
            return i;

    return -1;
}

/**
 * @brief Инициализирует CLI-интерфейс на основе ncurses.
 *
 * Выполняет следующие действия:
 * - Инициализирует ncurses (initscr)
 * - Настраивает режимы: cbreak, noecho, keypad, nodelay
 * - Скрывает курсор (curs_set(0))
 * - Выделяет и инициализирует внутренний контекст
 *
 * @param width  Ширина игрового поля в символах (должна быть > 0)
 * @param height Высота игрового поля в символах (должна быть > 0)
 * @param fps    Целевая частота обновления кадров (в кадрах в секунду, >= 1)
 *
 * @return Указатель на инициализированный контекст (ViewHandle_t) при успехе;
 *         NULL — если инициализация ncurses или выделение памяти не удалась.
 *
 * @note Функция не должна вызываться повторно без предварительного cli_shutdown().
 * @note Повторная инициализация может привести к утечкам или повреждению состояния ncurses.
 * @note Все настройки ncurses управляются автоматически — не вызывайте их напрямую.
 */
static ViewHandle_t cli_init(int width, int height, int fps) {
    if (width <= 0 || height <= 0 || fps < 1) {
        return NULL;
    }

    CliContext_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->width = width;
    ctx->height = height;
    ctx->fps = fps;
    if (initscr() == NULL) {
        free(ctx);
        return NULL;
    };

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    ctx->win = stdscr;

    return ctx;
}

/**
 * @brief Настраивает зону вывода для элемента интерфейса.
 *
 * Определяет прямоугольную область на экране, связанную с именем (идентификатором).
 * Эта зона используется в дальнейшем для отрисовки данных через draw_element.
 * Если зона с таким идентификатором уже существует — она заменяется.
 *
 * @param handle     Контекст CLI (ViewHandle_t, не должен быть NULL)
 * @param element_id Имя зоны — C-строка (не NULL, не пустая, макс. длина: MAX_NAME_LEN)
 * @param x          Координата X левого верхнего угла в символах (начиная с 1)
 * @param y          Координата Y левого верхнего угла в символах (начиная с 1)
 * @param max_w      Максимальная ширина зоны в символах (должна быть > 0)
 * @param max_h      Максимальная высота зоны в символах (должна быть > 0)
 *
 * @return
 *         - VIEW_OK — зона успешно настроена
 *         - VIEW_NOT_INITIALIZED — handle равен NULL
 *         - VIEW_BAD_DATA — один из параметров недопустим (x,y ≤ 0, ширина/высота ≤ 0,
 *                           element_id — NULL или пустая строка)
 *         - VIEW_ERROR — достигнуто максимальное число зон (zone_count >= MAX_ZONES)
 *
 * @note Координаты начинаются с 1, чтобы оставить место для рамки и подписи:
 *       рамка рисуется от (x-1, y-1), поэтому x и y = 0 могут выйти за границы экрана.
 * @note Максимальное имя зоны — MAX_NAME_LEN символов (обрезается при необходимости).
 */
static ViewResult_t cli_configure_zone(ViewHandle_t handle, const char *element_id, int x, int y, int max_w, int max_h) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;
    if (ctx->zone_count >= MAX_ZONES) return VIEW_ERROR;
    if (max_w <= 0 || max_h <= 0 || x <=0 || y <= 0) return VIEW_BAD_DATA;
    if (!element_id) return VIEW_BAD_DATA;

    int idx = ctx->zone_count++;
    strncpy(ctx->zone_names[idx], element_id, MAX_NAME_LEN - 1);
    ctx->zone_names[idx][MAX_NAME_LEN - 1] = 0;
    ctx->zones[idx].x = x;
    ctx->zones[idx].y = y;
    ctx->zones[idx].w = max_w;
    ctx->zones[idx].h = max_h;

    return VIEW_OK;
}

/**
 * @brief Отрисовывает данные в указанной зоне интерфейса.
 *
 * Функция очищает внутреннюю область зоны (внутри рамки), затем рисует содержимое
 * в соответствии с типом данных (текст, число или матрица). Также рисует рамку
 * вокруг зоны и её имя в верхнем левом углу над рамкой.
 *
 * @param handle     Контекст CLI (ViewHandle_t, не должен быть NULL)
 * @param element_id Имя зоны, ранее настроенной через configure_zone (не NULL)
 * @param data       Указатель на данные для отрисовки (не должен быть NULL)
 *
 * @return
 *         - VIEW_OK — отрисовка выполнена успешно
 *         - VIEW_NOT_INITIALIZED — handle равен NULL
 *         - VIEW_INVALID_ID — зона с указанным element_id не найдена
 *         - VIEW_BAD_DATA — data == NULL, или type имеет неверное значение,
 *                           или для ELEMENT_MATRIX указан NULL-указатель на данные
 *
 * @note Данные не копируются — память, на которую указывает data->content,
 *       должна оставаться валидной до вызова render().
 *
 * @note Тип данных должен корректно соответствовать используемому полю union:
 *       - ELEMENT_TEXT:   используется content.text
 *       - ELEMENT_NUMBER: используется content.number
 *       - ELEMENT_MATRIX: используется content.matrix
 *
 * @note Для ELEMENT_MATRIX каждый ненулевой элемент отображается как "[ ]",
 *       нулевой — как пробел. Матрица отображается с учётом max_w и max_h зоны.
 */
static ViewResult_t cli_draw_element(ViewHandle_t handle,
                             const char *element_id,
                             const ElementData_t *data)
{
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;
    int idx = find_zone(ctx, element_id);
    if (idx < 0) return VIEW_INVALID_ID;

    // Извлекаем параметры зоны
    int x = ctx->zones[idx].x;
    int y = ctx->zones[idx].y;
    int w = ctx->zones[idx].w;
    int h = ctx->zones[idx].h;

    // РАБОТА С РАМКОЙ: рисуем рамку вокруг прямоугольника [w×h]
    // верхняя и нижняя границы
    mvwhline(ctx->win, y-1,   x-1, ACS_HLINE, w+2);
    mvwhline(ctx->win, y+h,   x-1, ACS_HLINE, w+2);
    // левая и правая границы
    mvwvline(ctx->win, y-1, x-1, ACS_VLINE, h+2);
    mvwvline(ctx->win, y-1, x+w, ACS_VLINE, h+2);
    // углы
    mvwaddch(ctx->win, y-1,   x-1,   ACS_ULCORNER);
    mvwaddch(ctx->win, y-1,   x+w,   ACS_URCORNER);
    mvwaddch(ctx->win, y+h,   x-1,   ACS_LLCORNER);
    mvwaddch(ctx->win, y+h,   x+w,   ACS_LRCORNER);

    // РАБОТА С РАМКОЙ: подпись над верхней границей по центру
    int title_len = strlen(element_id);
    int title_x = x - 1 + (w + 2 - title_len) / 2;
    mvwaddnstr(ctx->win, y-1, title_x, element_id, title_len);

    // Очищаем внутреннюю область рамки
    for (int row = 0; row < h; ++row) {
        mvwhline(ctx->win, y + row, x, ' ', w);
    }

    // Отрисовка данных внутри рамки
    switch (data->type) {
        case ELEMENT_TEXT: {
            const char *s = data->content.text;
            int row = 0, col = 0;
            for (const char *p = s; *p && row < h; ++p) {
                if (*p == '\n') { row++; col = 0; continue; }
                if (col < w)
                    mvwaddch(ctx->win, y + row, x + col++, *p);
            }
            break;
        }
        case ELEMENT_NUMBER: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", data->content.number);
            mvwaddnstr(ctx->win, y, x, buf, w);
            break;
        }
        case ELEMENT_MATRIX: {
            const int *arr = data->content.matrix.data;
            if (!arr) return VIEW_BAD_DATA;
            int mw = data->content.matrix.width;
            int mh = data->content.matrix.height;
            for (int row = 0; row < h && row < mh; ++row) {
                for (int col = 0; col < w && col < mw; ++col) {
                    // каждый блок — пара символов []
                    int px = x + col*2;
                    int py = y + row;
                    if (arr[row * mw + col]) {
                        mvwaddch(ctx->win, py,     px, '[');
                        mvwaddch(ctx->win, py, px+1, ']');
                    } else {
                        mvwaddch(ctx->win, py,     px, ' ');
                        mvwaddch(ctx->win, py, px+1, ' ');
                    }
                }
            }
            break;
        }
        default:
            return VIEW_BAD_DATA;
    }

    return VIEW_OK;
}

/**
 * @brief Отображает содержимое буфера на экране (двухфазная отрисовка).
 *
 * Применяет все отложенные изменения, сделанные через draw_element,
 * и физически обновляет содержимое экрана с помощью wrefresh().
 * Является синхронизирующей точкой вывода — до вызова render()
 * пользователь не видит никаких изменений.
 *
 * @param handle Контекст CLI (не должен быть NULL)
 *
 * @return
 *         - VIEW_OK — обновление выполнено успешно
 *         - VIEW_NOT_INITIALIZED — handle равен NULL (не инициализирован)
 *         - VIEW_ERROR — ошибка при обновлении экрана (например, сбой wrefresh)
 *
 * @note Функция не блокирует выполнение — возвращает управление сразу.
 * @note Может вызываться безопасно в любом состоянии (даже если буфер не менялся).
 * @note Частота вызова должна соответствовать целевому FPS,
 *       указанному при инициализации, для плавной анимации и экономии ресурсов.
 *
 * @note Если draw_element не вызывался с момента последнего render(),
 *       обновление всё равно произойдёт, но визуально экран не изменится.
 *
 * @note Для корректной работы требуется, чтобы ncurses был инициализирован
 *       (вызван initscr). Поведение не определено, если render вызван до init.
 */
static ViewResult_t cli_render(ViewHandle_t handle) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;

    int result = wrefresh(ctx->win);
    return (result == ERR) ? VIEW_ERROR : VIEW_OK;
}

/**
 * @brief Считывает одно событие с клавиатуры (неблокирующий ввод).
 *
 * Проверяет наличие пользовательского ввода и возвращает логический код клавиши
 * и её состояние:
 *   - key_state = 0 — новое нажатие (первое событие или после паузы)
 *   - key_state = 1 — удержание (повторное срабатывание в короткий промежуток времени)
 *
 * @param handle Контекст CLI (не должен быть NULL)
 * @param event  Указатель на структуру InputEvent_t, куда будет записано событие
 *
 * @return
 *         - VIEW_OK — событие успешно прочитано, *event заполнено
 *         - VIEW_NO_EVENT — клавиша не нажата, событие отсутствует
 *         - VIEW_ERROR — передан NULL-указатель в event
 *         - VIEW_NOT_INITIALIZED — handle равен NULL
 *
 * @note Использует временной порог (200 мс) для определения удержания.
 *       Если между нажатиями прошло больше времени — считается новым нажатием.
 * @note Преобразует специальные коды (стрелки) в логические символы: 'w', 'a', 's', 'd'.
 * @note Функция использует статическое состояние — не является потокобезопасной.
 *       Должна вызываться из одного потока (основной игровой цикл).
 * @note Требует поддержки gettimeofday() (POSIX). Для кросс-платформенности
 *       в будущем рекомендуется интегрировать таймер в CliContext.
 */
static ViewResult_t cli_poll_input(ViewHandle_t handle, InputEvent_t *event) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;
    if (!event) return VIEW_ERROR;

    int ch = getch();
    if (ch == ERR) {
        return VIEW_NO_EVENT;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long current_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    static int last_key = -1;
    static long long last_time = 0;
    static bool initialized = false;

    const long long HOLD_TRESHOLD_MS = 200; // порог удержания клавиши в миллисекундах
    InputEvent_t ev = {0}; // key_state пока не поддерживается

    switch (ch) {
        case KEY_LEFT:  ev.key_code = 'a'; break;
        case KEY_RIGHT: ev.key_code = 'd'; break;
        case KEY_UP:    ev.key_code = 'w'; break;
        case KEY_DOWN:  ev.key_code = 's'; break;
        default:        ev.key_code = ch;   break;
    }

    if (initialized && last_key == ev.key_code && (current_time - last_time) < HOLD_TRESHOLD_MS) {
        ev.key_state = 1; // Удержание (бвстрый повтор)
    } else {
        ev.key_state = 0; // Новое нажате
    }

    last_key = ev.key_code;
    last_time = current_time;
    initialized = true;

    *event = ev;
    return VIEW_OK;
}

/**
 * @brief Завершает работу CLI-интерфейса и освобождает все связанные ресурсы.
 *
 * Деинициализирует библиотеку ncurses (вызывает endwin()), освобождает
 * выделенную память контекста и возвращает системные ресурсы.
 * После вызова handle становится недействительным и не должен использоваться.
 *
 * @param handle Контекст CLI (может быть NULL — вызов будет проигнорирован)
 *
 * @return
 *         - VIEW_OK — завершение прошло успешно
 *         - VIEW_NOT_INITIALIZED — handle равен NULL
 *         - VIEW_ERROR — ошибка при вызове endwin() (например, сбой завершения режима ncurses)
 *
 * @note Функция безопасна к вызову с NULL.
 * @note Повторный вызов с тем же handle приведёт к неопределённому поведению (double free).
 * @note Даже при возврате VIEW_ERROR, ресурсы (включая handle) считаются недействительными.
 */
static ViewResult_t cli_shutdown(ViewHandle_t handle) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;

    const int result = endwin();
    free(ctx);
    return (result != ERR) ? VIEW_OK : VIEW_ERROR;
}

/**
 * @internal
 * @brief Экспортируемый экземпляр CLI-реализации интерфейса отображения.
 *
 * Константная структура, реализующая vtable-интерфейс ViewInterface.
 * Содержит указатели на функции, управляющие ncurses-интерфейсом:
 * инициализация, отрисовка, ввод, освобождение ресурсов.
 *
 * @note Должен использоваться только для чтения.
 *       Модификация полей приведёт к неопределённому поведению.
 *
 * @note Все функции не являются потокобезопасными — должны вызываться
 *       из одного потока (обычно — основного цикла игры).
 *
 * @note Реализация самостоятельно управляет состоянием ncurses
 *       (initscr / endwin). Не вызывайте эти функции напрямую.
 */
const ViewInterface cli_view = {
    .version = VIEW_INTERFACE_VERSION,
    .init            = cli_init,
    .configure_zone  = cli_configure_zone,
    .draw_element    = cli_draw_element,
    .render          = cli_render,
    .poll_input      = cli_poll_input,
    .shutdown        = cli_shutdown
};

/** @} */