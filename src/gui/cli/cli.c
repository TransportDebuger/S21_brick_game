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
 * @note Реализация корректно обрабатывает повторную инициализацию
 *       и деинициализацию интерфейса за счёт отслеживания состояния ncurses.
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
 * @internal
 * @def MAX_ZONES
 * @brief Максимальное количество поддерживаемых зон интерфейса.
 *
 * Определяет верхнюю границу числа элементов, которые можно настроить
 * через configure_zone. Используется для фиксации размера внутренних
 * массивов (например, zone_names, zones).
 */
#define MAX_ZONES 8

/**
 * @internal
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

#define MAX_COLOR_PAIRS 8
#define DEFAULT_FG_COLOR  COLOR_BLUE
#define DEFAULT_BG_COLOR  COLOR_BLACK

/**
 * @internal
 * @typedef CliContext_t
 * @brief Тип для внутреннего контекста CLI-модуля.
 *
 * Представляет собой opaque-указатель на внутренние данные интерфейса.
 * Используется как реализация ViewHandle_t — абстрактного контекста представления.
 */
typedef struct CliContext CliContext_t;

/**
 * @internal
 * @struct CliContext
 * @brief Внутренний контекст CLI-модуля.
 *
 * Содержит все данные, необходимые для работы CLI-интерфейса:
 * - Габариты игрового поля и целевую частоту кадров (fps).
 * - Конфигурацию зон отображения: позиции, размеры и имена.
 * - Указатель на окно ncurses (stdscr).
 * - Состояние ввода: последняя клавиша и временная метка для определения удержания.
 *
 * @var CliContext::width
 *     Ширина игрового поля в символах (количество столбцов).
 * @var CliContext::height
 *     Высота игрового поля в символах (количество строк).
 * @var CliContext::fps
 *     Целевая частота обновления экрана (кадров в секунду).
 * @var CliContext::zones
 *     Массив прямоугольных зон отображения (максимум MAX_ZONES).
 *     Каждая зона задаётся координатами (x, y) и размерами (w, h).
 * @var CliContext::zone_names
 *     Имена зон (идентификаторы) для поиска по строковому ключу.
 *     Длина имени ограничена MAX_NAME_LEN.
 * @var CliContext::zone_count
 *     Текущее количество настроенных зон (от 0 до MAX_ZONES-1).
 * @var CliContext::win
 *     Указатель на главное окно ncurses (обычно stdscr).
 *     Используется для всех операций отрисовки.
 * @var CliContext::last_input_key
 *     Логический код последней нажатой клавиши (для определения удержания).
 * @var CliContext::last_input_time
 *     Временная метка последнего ввода в миллисекундах (для тайминга удержания).
 * @var CliContext::input_initialized
 *     Флаг, указывающий, было ли уже инициализировано состояние ввода.
 *     Используется для корректной обработки первого нажатия.
 */
struct CliContext {
    int width, height, fps;
    struct {
        int x, y, w, h;
    } zones[MAX_ZONES]; // максимум 8 зон интерфейса
    char zone_names[MAX_ZONES][MAX_NAME_LEN];
    int zone_count;
    WINDOW *win;
    int last_input_key;
    long long last_input_time;
    bool input_initialized;
};

/**
 * @internal
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
 * Выполняет полную инициализацию текстового интерфейса:
 * - Инициализирует библиотеку ncurses с помощью initscr().
 * - Настраивает терминал: включает режим cbreak, отключает эхо ввода,
 *   активирует расширенные клавиши (keypad) и неблокирующий ввод (nodelay).
 * - Скрывает курсор для улучшения визуального восприятия.
 * - Выделяет и инициализирует внутренний контекст CliContext.
 *
 * @param width  Ширина игрового поля в символах; должно быть больше 0.
 * @param height Высота игрового поля в символах; должно быть больше 0.
 * @param fps    Целевая частота обновления экрана в кадрах в секунду; должно быть >= 1.
 *
 * @return Указатель на инициализированный контекст (ViewHandle_t) при успехе;
 *         NULL — если один из параметров недопустим, невозможно инициализировать ncurses
 *         или произошла ошибка выделения памяти.
 *
 * @note Функция является идемпотентной в пределах жизненного цикла ncurses:
 *       повторный вызов без предварительного shutdown приведёт к ошибке.
 * @note После успешного shutdown можно безопасно вызвать init снова.
 * @note Все настройки ncurses управляются внутренне — не вызывайте initscr,
 *       cbreak, keypad и другие функции напрямую.
 */
static ViewHandle_t cli_init(int width, int height, int fps) {
    if (width <= 0 || height <= 0 || fps < 1) {
        return NULL;
    }

    CliContext_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    
    bool ncurses_was_initialized = (stdscr != NULL);
    if (!ncurses_was_initialized) {
        if (initscr() == NULL) {
            free(ctx);
            return NULL;
        };
    };

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        // Создаём цветовую пару по умолчанию: синий текст на чёрном фоне
        init_pair(1, DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
        // Дополнительные цвета для матрицы: 2-16
        // например: 2 = красный, 3 = зелёный и т.д.
        // можно задать через конфиг или по умолчанию
        init_pair(2, COLOR_RED,    COLOR_BLACK);
        init_pair(3, COLOR_GREEN,  COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_CYAN,   COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA,COLOR_BLACK);
        init_pair(7, COLOR_BLUE,   COLOR_BLACK);
        init_pair(8, COLOR_WHITE,  COLOR_BLACK);
    } 
    
    ctx->width = width;
    ctx->height = height;
    ctx->fps = fps;
    ctx->last_input_key = -1;
    ctx->last_input_time = 0;
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
    if (!element_id || strlen(element_id) == 0) return VIEW_BAD_DATA;

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
 * @note Для ELEMENT_MATRIX каждый ненулевой элемент отображается как "[]",
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

    int x = ctx->zones[idx].x;
    int y = ctx->zones[idx].y;
    int w = ctx->zones[idx].w;
    int h = ctx->zones[idx].h;
    WINDOW *win = ctx->win;

    // --- РАМКА И ЗАГОЛОВОК (цвет по умолчанию) ---
    wattron(win, COLOR_PAIR(1));  // стандартный цвет

    mvwhline(win, y-1,   x-1, ACS_HLINE, w+2);
    mvwhline(win, y+h,   x-1, ACS_HLINE, w+2);
    mvwvline(win, y-1, x-1, ACS_VLINE, h+2);
    mvwvline(win, y-1, x+w, ACS_VLINE, h+2);
    mvwaddch(win, y-1,   x-1,   ACS_ULCORNER);
    mvwaddch(win, y-1,   x+w,   ACS_URCORNER);
    mvwaddch(win, y+h,   x-1,   ACS_LLCORNER);
    mvwaddch(win, y+h,   x+w,   ACS_LRCORNER);

    int title_len = strlen(element_id);
    int title_x = x - 1 + (w + 2 - title_len) / 2;
    mvwaddnstr(win, y-1, title_x, element_id, title_len);

    wattroff(win, COLOR_PAIR(1));

    // --- ОЧИСТКА ВНУТРЕННЕЙ ОБЛАСТИ ---
    for (int row = 0; row < h; ++row) {
        mvwhline(win, y + row, x, ' ', w);
    }

    // --- ОТРИСОВКА ДАННЫХ ---
    switch (data->type) {
        case ELEMENT_TEXT: {
            wattron(win, COLOR_PAIR(1));  // стандартный цвет
            const char *s = data->content.text;
            int row = 0, col = 0;
            for (const char *p = s; *p && row < h; ++p) {
                if (*p == '\n') { row++; col = 0; continue; }
                if (col < w)
                    mvwaddch(win, y + row, x + col++, *p);
            }
            wattroff(win, COLOR_PAIR(1));
            break;
        }
        case ELEMENT_NUMBER: {
            wattron(win, COLOR_PAIR(1));
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", data->content.number);
            mvwaddnstr(win, y, x, buf, w);
            wattroff(win, COLOR_PAIR(1));
            break;
        }
        case ELEMENT_MATRIX: {
            const int *arr = data->content.matrix.data;
            if (!arr) return VIEW_BAD_DATA;
            int mw = data->content.matrix.width;
            int mh = data->content.matrix.height;
            for (int row = 0; row < h && row < mh; ++row) {
                for (int col = 0; col < w && col < mw; ++col) {
                    int value = arr[row * mw + col];
                    int px = x + col*2;
                    int py = y + row;

                    // Если значение == 0 — не рисуем (фон)
                    if (value == 0) {
                        mvwaddch(win, py,     px, ' ');
                        mvwaddch(win, py, px+1, ' ');
                    } else {
                        // Используем значение как индекс цветовой пары
                        short color_pair = (short)(value % (MAX_COLOR_PAIRS-1) + 1); // 1..15
                        wattron(win, COLOR_PAIR(color_pair));
                        mvwaddch(win, py,     px, '[');
                        mvwaddch(win, py, px+1, ']');
                        wattroff(win, COLOR_PAIR(color_pair));
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
 *       (вызван init). Поведение не определено, если render вызван до init.
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

    const long long HOLD_THRESHOLD_MS = 200; // ✅ Исправлено: TRESHOLD → THRESHOLD
    InputEvent_t ev = {0};

    // Преобразование специальных клавиш
    switch (ch) {
        case KEY_LEFT:  ev.key_code = 'a'; break;
        case KEY_RIGHT: ev.key_code = 'd'; break;
        case KEY_UP:    ev.key_code = 'w'; break;
        case KEY_DOWN:  ev.key_code = 's'; break;
        default:        ev.key_code = ch;   break;
    }

    // Проверка удержания — только если инициализировано и в пределах порога
    if (ctx->input_initialized &&
        ctx->last_input_key == ev.key_code &&
        (current_time - ctx->last_input_time) < HOLD_THRESHOLD_MS) {
        ev.key_state = 1; // Удержание
    } else {
        ev.key_state = 0; // Новое нажатие
    }

    // Обновление состояния
    ctx->last_input_key = ev.key_code;
    ctx->last_input_time = current_time;
    ctx->input_initialized = true;

    *event = ev;
    return VIEW_OK;
}

/**
 * @brief Завершает работу CLI-интерфейса и освобождает все связанные ресурсы.
 *
 * Корректно завершает сессию работы с ncurses:
 * - Деинициализирует библиотеку с помощью endwin(), восстанавливая исходное состояние терминала.
 * - Освобождает память, выделенную под внутренний контекст (CliContext).
 * - При успешном завершении endwin() сбрасывает флаг активности ncurses_active, что позволяет
 *   безопасно повторно инициализировать интерфейс через cli_init().
 *
 * После вызова функции переданный handle становится недействительным и не должен использоваться повторно,
 * даже если функция вернула VIEW_ERROR.
 *
 * @param handle Указатель на контекст CLI (ViewHandle_t). Допустимо значение NULL — функция игнорирует вызов без последствий.
 *
 * @return
 *         - VIEW_OK — завершение прошло успешно: ncurses деинициализирован, память освобождена.
 *         - VIEW_NOT_INITIALIZED — handle равен NULL; операция не выполнена, но это не ошибка.
 *         - VIEW_ERROR — сбой при вызове endwin() (например, ошибка восстановления терминала).
 *
 * @note Вызов с NULL допустим и не приводит к аварийному завершению (idempotent по NULL).
 * @note Повторный вызов с тем же валидным handle приведёт к неопределённому поведению, включая двойное освобождение памяти.
 * @note Контекст считается недействительным сразу после первого вызова shutdown — дальнейшее использование запрещено.
 * @note Сброс флага ncurses_active происходит ТОЛЬКО при успешном выполнении endwin() — это гарантирует целостность состояния.
 */
static ViewResult_t cli_shutdown(ViewHandle_t handle) {
    CliContext_t *ctx = handle;
    if (!ctx) return VIEW_NOT_INITIALIZED;

    const int result = endwin();
    if (result != ERR) {
        free(ctx);
    }

    return (result != ERR) ? VIEW_OK : VIEW_ERROR;
}

/**
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