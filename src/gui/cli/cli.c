/**
 * @file cli.c
 * @brief Реализация CLI-интерфейса BrickGame на ncurses
 *
 * Все функции соответствуют контракту @ref ViewInterface из view.h.
 * Интерфейс полностью изолирован от модели игры.
 * 
 * @defgroup CliView Реализация библиотеки Cli интерфейса
 * @ingroup View
 * 
 * @author provemet
 * @date December 2024
 * @{
 */

#include "cli.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

/**
 * @def MAX_ZONES
 * @brief Максимальное число зон интерфейса.
 */
#define MAX_ZONES 8

/**
 * @def MAX_NAME_LEN
 * @brief Максимальная длина имени зоны.
 */
#define MAX_NAME_LEN 15

/**
 * @typedef CliContext_t
 * @brief Объявление типа для структуры внутреннего контекста модуля.
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
    } zones[8]; // максимум 8 зон интерфейса
    char zone_names[8][16];
    int zone_count;
    WINDOW *win;
};

/**
 * @brief Найти индекс зоны по её имени.
 * @param ctx Указатель на контекст CLI.
 * @param name Имя зоны (строка, до 15 символов).
 * @return Индекс зоны (0..zone_count-1) или -1, если не найдена.
 */
static int find_zone(CliContext_t *ctx, const char *name) {
    if (!ctx || !name) return -1;

    for (int i = 0; i < ctx->zone_count; ++i)
        if (strcmp(ctx->zone_names[i], name) == 0)
            return i;

    return -1;
}

/**
 * @brief Инициализация CLI-интерфейса.
 *
 * Функция настраивает режим ncurses, скрывает курсор,
 * включает неблокирующий ввод и сохраняет параметры.
 *
 * @param width  Ширина поля (столбцы).
 * @param height Высота поля (строки).
 * @param fps    Частота обновления (кадров/с).
 * @return Контекст CLI (ViewHandle_t) или NULL при ошибке.
 */
static ViewHandle_t cli_init(int width, int height, int fps) {
    CliContext_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->width = width;
    ctx->height = height;
    ctx->fps = fps;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    ctx->win = stdscr;

    return ctx;
}

/**
 * @brief Настроить зону вывода для элемента интерфейса.
 *
 * Зона определяется именем и прямоугольником на экране.
 *
 * @param handle     Контекст CLI (ViewHandle_t).
 * @param element_id Строковый идентификатор зоны (например, "field").
 * @param x          Координата X левого верхнего угла.
 * @param y          Координата Y левого верхнего угла.
 * @param max_w      Максимальная ширина зоны.
 * @param max_h      Максимальная высота зоны.
 * @return VIEW_OK при успехе, VIEW_ERROR при переполнении числа зон.
 */
static ViewResult_t cli_configure_zone(ViewHandle_t handle, const char *element_id, int x, int y, int max_w, int max_h) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;
    if (ctx->zone_count >= MAX_ZONES) return VIEW_ERROR;
    if (max_w <= 0 || max_h <= 0 || x <=0 || y <= 0) return VIEW_BAD_DATA;
    if (!element_id) return VIEW_BAD_DATA;

    int idx = ctx->zone_count++;
    strncpy(ctx->zone_names[idx], element_id, MAX_NAME_LEN);
    ctx->zone_names[idx][MAX_NAME_LEN] = 0;
    ctx->zones[idx].x = x;
    ctx->zones[idx].y = y;
    ctx->zones[idx].w = max_w;
    ctx->zones[idx].h = max_h;

    return VIEW_OK;
}

/**
 * @brief Отрисовать один элемент в заданной зоне.
 *
 * Очищает область, затем в зависимости от типа данных
 * рисует текст, число или матрицу символов.
 *
 * @param handle     Контекст CLI.
 * @param element_id Имя ранее настроенной зоны.
 * @param data       Данные для отрисовки (@ref ElementData_t).
 * 
 * @return Следующие значения:
 *         - VIEW_OK метод штатно отработал
 *         - VIEW_INVALID_ID не верный идентификатор зоны (зона с таким идентификатором не найдена)
 *         - VIEW_BAD_DATA не допустимый идентификатор типа данных.
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

    // <<< РАБОТА С РАМКОЙ: рисуем рамку вокруг прямоугольника [w×h]
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

    // <<< РАБОТА С РАМКОЙ: подпись над верхней границей по центру
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
 * @brief Показать собранный буфер на экране.
 *
 * Все изменения, сделанные через draw_element, отображаются
 * на экране после вызова render().
 *
 * @param handle Контекст CLI.
 */
static ViewResult_t cli_render(ViewHandle_t handle) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;
    wrefresh(ctx->win);
    return VIEW_OK;
}

/**
 * @brief Считать одно событие клавиатуры.
 *
 * Возвращает код нажатой клавиши и флаг удержания.
 * Если клавиша не нажата, возвращается {0,0}.
 *
 * @param handle Контекст CLI.
 * @return Структура @ref InputEvent_t с результатом.
 */
static ViewResult_t cli_poll_input(ViewHandle_t handle, InputEvent_t *event) {
    (void)handle; // unused
    if (!event) return VIEW_ERROR; // invalid argument

    int ch = getch();
    InputEvent_t ev = { .key_code = 0, .key_state = 0 };
    if (ch == ERR) {
        ev.key_code = 0;
        ev.key_state = 0;
        return VIEW_NO_EVENT;
    }
    switch (ch) {
        case KEY_LEFT:  ev.key_code = 'a'; break;
        case KEY_RIGHT: ev.key_code = 'd'; break;
        case KEY_UP:    ev.key_code = 'w'; break;
        case KEY_DOWN:  ev.key_code = 's'; break;
        default:        ev.key_code = ch;   break;
    }
    
    /** @todo доделать признак зажатой клавиши */
    // ev.key_state = 0;
    *event = ev;
    return VIEW_OK;
}

/**
 * @brief Завершить работу CLI и освободить ресурсы.
 *
 * Вызывает endwin() для ncurses и освобождает память контекста.
 *
 * @param handle Контекст CLI.
 */
static ViewResult_t cli_shutdown(ViewHandle_t handle) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;

    endwin();
    free(ctx);

    return VIEW_OK;
}

/**
 * @brief Экспортируемый экземпляр CLI-интерфейса.
 *
 * Содержит указатели на все функции реализации.
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