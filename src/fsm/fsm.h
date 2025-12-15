/**
 * @file fsm.h
 * @defgroup FSM Универсальная машина конечных автоматов
 * @brief Универсальная машина конечных автоматов
 *
 * Библиотека реализует абстрактный API для определения своих
 * состояний и событий в пользовательском коде (игровая логика).
 *
 * ### Пример использования
 *
 * @code
 * enum {
 *   EVT_START = 1,
 *   EVT_PAUSE,
 *   EVT_EXIT
 * };
 *
 * enum {
 *   STATE_INIT = 0,
 *   STATE_RUNNING,
 *   STATE_PAUSED,
 *   STATE_FINISHED
 * };
 *
 * typedef struct {
 *   int score;
 *   bool paused;
 * } GameContext;
 *
 * void on_enter_running(fsm_context_t ctx) {
 *   GameContext *g = (GameContext *)ctx;
 *   g->paused = false;
 * }
 *
 * void on_exit_paused(fsm_context_t ctx) {
 *   printf("Resume game\n");
 * }
 *
 * fsm_transition_t transitions[] = {
 *   {STATE_INIT, EVT_START, STATE_RUNNING, NULL, on_enter_running},
 *   {STATE_RUNNING, EVT_PAUSE, STATE_PAUSED, NULL, on_exit_paused},
 *   {STATE_PAUSED, EVT_START, STATE_RUNNING, on_exit_paused, on_enter_running},
 *   {STATE_RUNNING, FSM_EVENT_NONE, STATE_FINISHED, NULL, NULL}  // таймер
 * };
 *
 * fsm_t fsm;
 * GameContext ctx = {0};
 *
 * fsm_init(&fsm, &ctx, transitions, 4, STATE_INIT);
 * fsm_process_event(&fsm, EVT_START);
 * fsm_update(&fsm); // проверка на таймер (FSM_EVENT_NONE)
 * @endcode
 *
 * @author provemet
 * @date December 2024
 *
 * @{
 */

#ifndef FSM_H
#define FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @def FSM_EVENT_NONE
 * @brief Специальное событие, используемое для автоматических переходов.
 *
 * Передаётся как `event` в `fsm_transition_t`, чтобы обозначить переход без
 * внешнего триггера. Используется `fsm_update()` для поиска таких переходов.
 *
 * @note Рекомендуется использовать только одно правило на состояние.
 * @warning Не используйте значение данного определения для событий, определенных
 * в пользовательском коде.
 *
 * @code
 * {STATE_RUNNING, FSM_EVENT_NONE, STATE_TIMEOUT, NULL, on_enter_timeout}
 * @endcode
 */
#define FSM_EVENT_NONE 0

/**
 * @typedef fsm_event_t
 * @brief Тип идентификаторов событий (триггеров) FSM.
 *
 * Целочисленный тип для представления событий. Позволяет использовать
 * перечисления или именованные константы.
 * 
 * @warning Значение `0` зарезервировано под @ref FSM_EVENT_NONE.
 * Не используйте `0` для пользовательских событий.
 *
 * @see @ref FSM_EVENT_NONE
 * @see @ref fsm_process_event
 * @see @ref fsm_update
 */
typedef int fsm_event_t;

/**
 * @typedef fsm_state_t
 * @brief Тип идентификаторов состояний FSM.
 *
 * Целочисленный тип для представления состояний. Рекомендуется использовать
 * перечисления для типобезопасности.
 * 
 * @note Не рекомендуется использовать отрицательные значения.
 * 
 * @see @ref fsm_t
 */
typedef int fsm_state_t;

/**
 * @typedef fsm_context_t
 * @brief Указатель на пользовательский контекст FSM.
 *
 * Хранит указатель на данные приложения, передаваемые в колбэки.
 * Может быть NULL, если контекст не требуется.
 *
 * Пользователь может передавать любую структуру, например:
 * 
 * @code
   typedef struct {  
     custom_data_t data;  
     int count;  
   } MyContext;  
  
   MyContext ctx;  
   fsm_context_t fctx = &ctx;  
   fsm_init(&fsm, fctx, transitions, count, STATE_INIT);  
   @endcode
 * 
 * @see @ref fsm_t
 * @see @ref fsm_cb_t
 */
typedef void *fsm_context_t;

/**
 * @typedef fsm_cb_t
 * @brief Сигнатура callback-функции FSM.
 *
 * Вызывается при входе или выходе из состояния.
 *
 * @param ctx Указатель на пользовательский контекст (может быть NULL).
 *
 * @note Колбэки не должны вызывать `fsm_process_event` или `fsm_update`
 *       без защиты от рекурсии — флаг `processing` предотвращает это.
 * 
 * @see @ref fsm_transition_t
 * @see @ref fsm_init
 */
typedef void (*fsm_cb_t)(fsm_context_t ctx);

/**
 * @struct fsm_transition_t
 * @brief Описание перехода в конечном автомате.
 *
 * Задаёт правило: "из состояния `src` по событию `event` перейти в `dst`".
 *
 * @var fsm_transition_t::src
 *      Исходное состояние.
 * @var fsm_transition_t::event
 *      Событие, вызывающее переход.
 * @var fsm_transition_t::dst
 *      Целевое состояние.
 * @var fsm_transition_t::on_exit
 *      Колбэк при выходе из `src` (может быть NULL).
 * @var fsm_transition_t::on_enter
 *      Колбэк при входе в `dst` (может быть NULL).
 *
 * @note Порядок в массиве важен: при совпадении нескольких правил
 *       выполняется первое найденное.
 *
 * @see @ref fsm_t
 * @see @ref FSM_EVENT_NONE
 */
typedef struct {
  fsm_state_t src;
  fsm_event_t event;
  fsm_state_t dst;
  fsm_cb_t on_exit;
  fsm_cb_t on_enter;
} fsm_transition_t;

/**
 * @struct fsm_t
 * @brief Основная структура конечного автомата.
 *
 * Содержит текущее состояние, таблицу переходов и контекст.
 *
 * @var fsm_t::transitions
 *      Указатель на массив переходов (не владеет памятью).
 * @var fsm_t::count
 *      Количество элементов в массиве.
 * @var fsm_t::current
 *      Текущее состояние.
 * @var fsm_t::ctx
 *      Пользовательский контекст.
 * @var fsm_t::processing
 *      Флаг, предотвращающий рекурсивную обработку событий.
 *
 * @note Не изменяйте поля вручную — используйте API.
 *
 * @see @ref fsm_init
 * @see @ref fsm_process_event
 * @see @ref fsm_update
 */
typedef struct {
  const fsm_transition_t *transitions;
  size_t count;
  fsm_state_t current;
  fsm_context_t ctx;
  bool processing;
} fsm_t;

/**
 * @brief Инициализировать конечный автомат.
 *
 * @param[out] fsm         Указатель на структуру (не NULL).
 * @param[in]  ctx         Пользовательский контекст (может быть NULL).
 * @param[in]  transitions Массив переходов (не NULL).
 * @param[in]  count       Количество переходов (> 0).
 * @param[in]  start_state Начальное состояние.
 * @return true при успехе, false при ошибках.
 *
 * @pre fsm != NULL && transitions != NULL && count > 0
 * @post fsm->current == start_state && fsm->processing == false
 *
 * @note on_enter для start_state не вызывается.
 *
 * @see @ref fsm_destroy
 * @see @ref fsm_process_event
 */
bool fsm_init(fsm_t *fsm, fsm_context_t ctx,
              const fsm_transition_t *transitions, size_t count,
              fsm_state_t start_state);

/**
 * @brief Освободить ресурсы FSM (заглушка).
 *
 * В текущей реализации не делает ничего.
 * Добавлен для симметрии API и будущего расширения.
 *
 * @param[in,out] fsm Указатель на FSM (может быть NULL).
 * @return void Ничего не возвращает.
 *
 * @note Не освобождает память под fsm_t — это делает пользователь.
 * @note Функция безопасна при передаче NULL.
 *
 * @see @ref fsm_init
 */
void fsm_destroy(fsm_t *fsm);

/**
 * @brief Обработать событие и выполнить переход.
 *
 * Ищет переход с `src == current` и `event == event`.
 * Если найден — выполняет: on_exit → смена состояния → on_enter.
 *
 * @param[in,out] fsm   Указатель на FSM (не NULL).
 * @param[in]     event Событие.
 * @return true, если переход выполнен; false иначе.
 *
 * @pre fsm != NULL
 * 
 * @note Если `fsm == NULL`, возвращает false.
 *
 * @see @ref fsm_update
 * @see @ref fsm_process_event
 */
bool fsm_process_event(fsm_t *fsm, fsm_event_t event);

/**
 * @brief Выполнить автоматический переход (по FSM_EVENT_NONE).
 *
 * Ищет первый переход с `src == current` и `event == FSM_EVENT_NONE`.
 * Если найден — выполняет переход с колбэками.
 *
 * @param[in,out] fsm Указатель на FSM (не NULL).
 * @return void Ничего не возвращает.
 *
 * @pre fsm != NULL
 *
 * @note Используется для таймеров, условий и внутренних триггеров.
 * @note Функция безопасна при передаче NULL.
 *
 * @see @ref FSM_EVENT_NONE
 * @see @ref fsm_process_event
 */
void fsm_update(fsm_t *fsm);

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */

/** @} */  // end of FSM module