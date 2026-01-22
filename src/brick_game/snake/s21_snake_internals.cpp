/**
 * @file s21_snake_internals.cpp
 * @brief Реализация внутренней логики игры "Змейка" на C++17
 *
 * Содержит полную реализацию класса `s21::SnakeGame`, включая:
 * - управление состоянием через конечный автомат (FSM)
 * - движение, рост и повороты змейки
 * - генерацию яблок на свободных ячейках
 * - проверку столкновений с границами и собственным телом
 * - обновление счёта, уровня и скорости
 * - загрузку и сохранение рекорда между сессиями
 * - интеграцию с C API через непрозрачные указатели (void*)
 *
 * Архитектурные особенности:
 * - **Паттерн "Непрозрачный указатель"**: экземпляр `SnakeGame` скрыт от C API,
 *   доступ только через статические методы `create` / `destroy` / `handle_input`.
 * - **Жизненный цикл**: управление памятью строго инкапсулировано.
 * - **Модульная структура**: методы разделены по ответственностям:
 *   - Инициализация: `initialize_()`, `initializeSnake_()`
 *   - FSM и ввод: `handle_input()`, `mapActionToEvent_()`, `processEvent_()`
 *   - Логика игры: `move_()`, `checkCollision_()`, `eatApple_()`
 *   - Генерация: `spawnApple_()`
 *   - Синхронизация состояния: `updateFieldState_()`, `updateOccupiedCells_()`
 *   - Работа с сохранениями: `loadHighScore_()`, `saveHighScore_()`
 * - **FSM на основе таблицы переходов**: чёткая типизация через `enum class`.
 * - **Использование C++17**:
 *   - `std::deque<SnakeSegment>` — эффективное управление телом змейки
 *   - `std::unordered_set<int>` — O(1) проверка занятых ячеек (ключ: y * cols + x)
 *   - `thread_local std::mt19937` — потокобезопасная генерация яблок
 *   - `constexpr`, `noexcept`, `enum class` — безопасная, современная реализация
 *
 * @note Все публичные методы помечены `noexcept`: исключения не пересекают границу C.
 * @note Класс не предназначен для прямого использования вне `s21_snake.cpp`.
 * @note Потокобезопасность не гарантируется — все вызовы должны быть из одного потока.
 * @note Файл компилируется как часть C++ модуля и линкуется с C-обёрткой.
 *
 * @warning Не изменяйте логику FSM без синхронизации `transitions_` и `processEvent_()`.
 * @warning Не нарушайте инварианты `SnakeSegment`: (x, y) должны быть в пределах поля.
 * @warning Не удаляйте или не переупорядочивайте `SnakeState` и `SnakeEvent` — это нарушит ABI.
 *
 * @author provemet
 * @version 1.1
 * @date 2025-12-27
 * @see s21_snake_internals.hpp — объявление класса и типов
 * @see s21_snake_internals.cpp — реализация модели игры "Змейка"
 * @see s21_snake.cpp          — C API обёртка (extern "C")
 * @see s21_snake.h            — публичный C интерфейс для движка s21_bgame
 * @see GameInfo_t             — структура данных для отрисовки
 */


#include "s21_snake_internals.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <random>
#include <thread>  // для std::this_thread::get_id()
#include <vector>

namespace s21 {

/**
 * @internal
 * @brief Таблица переходов конечного автомата (FSM) для игры "Змейка"
 *
 * Определяет правила перехода между состояниями игры в ответ на события.
 * Каждая запись — это кортеж:
 * { текущее_состояние, событие, новое_состояние, on_exit, on_enter }
 *
 * Используется внутренним FSM (`fsm_t`) для диспетчеризации событий.
 * Все переходы сопровождаются вызовом колбэка `on_state_enter_`, который
 * передаёт управление в C++ контекст через статический метод.
 *
 * @note Порядок записей не влияет на логику — FSM проверяет все переходы
 * подряд.
 * @note Переходы исчерпывающие: все допустимые комбинации "состояние + событие"
 *       описаны явно.
 * @note Событие `TICK` (внутренний таймер) не имеет отдельного перехода —
 *       обрабатывается в колбэке `on_state_enter_` при состоянии `MOVE`.
 * @note Состояние `INIT` используется для начального экрана, перед стартом
 * игры.
 * @note Состояние `GAME_OVER` позволяет перезапустить игру при событии `START`.
 *
 * @warning Нельзя изменять порядок или удалять записи без пересмотра логики
 * FSM.
 * @warning Все указатели состояний и событий должны быть корректно
 * преобразованы через `to_fsm_state()` и `to_fsm_event()`.
 * @warning Колбэк `on_state_enter_` обязателен — без него не будет вызвана
 *          игровая логика (`processEvent_`).
 *
 * @par Пример логики переходов:
 * - INIT + START → MOVE: игра начинается, змейка появляется на поле.
 * - MOVE + PAUSE_TOGGLE → PAUSED: игра приостанавливается.
 * - PAUSED + PAUSE_TOGGLE → MOVE: игра возобновляется.
 * - MOVE + TERMINATE → GAME_OVER: игра завершается (например, по таймеру или
 * ошибке).
 * - GAME_OVER + START → INIT: игра сбрасывается, готова к новому запуску.
 */
const fsm_transition_t SnakeGame::transitions_[] = {
    // INIT → MOVE: запуск новой игры по START
    {to_fsm_state(SnakeState::INIT), to_fsm_event(SnakeEvent::START),
     to_fsm_state(SnakeState::MOVE), nullptr, &SnakeGame::on_state_enter_},

    // MOVE → PAUSED: игра приостанавливается по PAUSE_TOGGLE
    {to_fsm_state(SnakeState::MOVE), to_fsm_event(SnakeEvent::PAUSE_TOGGLE),
     to_fsm_state(SnakeState::PAUSED), nullptr, &SnakeGame::on_state_enter_},

    // PAUSED → MOVE: возобновление игры по PAUSE_TOGGLE
    {to_fsm_state(SnakeState::PAUSED), to_fsm_event(SnakeEvent::PAUSE_TOGGLE),
     to_fsm_state(SnakeState::MOVE), nullptr, &SnakeGame::on_state_enter_},

    // MOVE → GAME_OVER: завершение игры по TERMINATE (столкновение и др.)
    {to_fsm_state(SnakeState::MOVE), to_fsm_event(SnakeEvent::TERMINATE),
     to_fsm_state(SnakeState::GAME_OVER), nullptr, &SnakeGame::on_state_enter_},

    // GAME_OVER → INIT: сброс игры без события автоматически
    {to_fsm_state(SnakeState::GAME_OVER), to_fsm_event(SnakeEvent::AUTO_RESET),
     to_fsm_state(SnakeState::INIT), nullptr, &SnakeGame::on_state_enter_},

    // Событие TICK обрабатывается внутри on_state_enter_ в состоянии MOVE
    // Нет явного перехода — используется для регулярного обновления игры
};

#ifdef SNAKE_TEST_ACCESS
// Для тестирования функция установки еды.
void SnakeGame::set_food_for_testing(int x, int y) {
  apple_x_ = x;
  apple_y_ = y;
}
#endif

/**
 * @brief Создаёт новый экземпляр игры "Змейка"
 * @return void* Указатель на созданный экземпляр игры (непрозрачный контекст),
 * или nullptr при ошибке
 *
 * Выполняет полную инициализацию игры:
 * - выделяет память под объект SnakeGame
 * - вызывает конструктор для настройки состояния FSM, счёта, уровня и скорости
 * - инициализирует игровое поле и тело змейки
 * - загружает рекорд из файла (путь: SNAKE_SCORE_FILE)
 * - создаёт директорию сохранений, если она отсутствует (SNAKE_SCORE_DIR)
 *
 * @note Возвращает nullptr в случае:
 *       - нехватки памяти (оператор new выбросил исключение — не должно
 * происходить, т.к. nothrow)
 *       - отказа в доступе к файловой системе
 *       - повреждённого формата файла рекорда
 * @note Указатель должен быть передан в snake_destroy() для корректного
 * освобождения ресурсов.
 * @note Функция потоконебезопасна: не вызывайте одновременно из нескольких
 * потоков.
 *
 * @warning Не используйте free/delete на возвращённом указателе — только
 * snake_destroy().
 * @warning Повторный вызов без предварительного уничтожения приведёт к утечке
 * памяти.
 *
 * @par Пример:
 * @code
 * void* game = snake_create();
 * if (!game) {
 *     fprintf(stderr, "Ошибка: не удалось создать игру\n");
 *     return -1;
 * }
 * // ... использование игры
 * snake_destroy(game);  // Обязательно освободите память
 * @endcode
 */
void* SnakeGame::create() noexcept {
  try {
    return new SnakeGame();
  } catch (const std::bad_alloc&) {
    return nullptr;
  } catch (...) {
    // Это маловероятно, но на случай других исключений
    return nullptr;
  }
}

/**
 * @brief Уничтожает экземпляр игры и освобождает все связанные ресурсы
 * @param[in] game Указатель на экземпляр игры. Если равен nullptr, функция
 * ничего не делает.
 *
 * Выполняет корректное завершение игры:
 * - сохраняет текущий рекорд (high_score) в файл (SNAKE_SCORE_FILE)
 * - освобождает динамическую память, выделенную под:
 *   - основное игровое поле (info_.field)
 *   - буфер next-фигуры (info_.next, если используется)
 *   - сам объект SnakeGame
 * - вызывает деструктор, который гарантирует освобождение всех внутренних
 * ресурсов
 *
 * @note Функция идемпотентна: многократный вызов с тем же указателем (после
 * первого destroy) приведёт к неопределённому поведению (доступ к освобождённой
 * памяти).
 * @note Безопасна для вызова с nullptr — игнорирует вызов.
 * @note Вызов может изменить состояние файловой системы (запись рекорда).
 * @note Потоконебезопасна: не вызывайте одновременно с другими функциями API
 * для одного экземпляра.
 *
 * @warning Не используйте указатель `game` после вызова — он становится
 * недействительным.
 * @warning Не вызывайте free/delete на `game` — память уже освобождена через
 * delete.
 * @warning Не сохраняйте копии указателя — все ссылки становятся "висячими".
 *
 * @par Пример:
 * @code
 * void* game = snake_create();
 * if (!game) {
 *   // обработка ошибки
 * }
 *
 * // ... игра
 *
 * snake_destroy(game);  // Рекорд сохранён, память освобождена
 * game = nullptr;       // Хорошая практика: предотвращение повторного
 * использования
 * @endcode
 */
void SnakeGame::destroy(void* game) noexcept {
  if (game != nullptr) {
    delete static_cast<SnakeGame*>(game);
  }
}

/**
 * @brief Обрабатывает пользовательский ввод в игре
 * @param[in] game Указатель на экземпляр игры. Если равен nullptr, функция
 * ничего не делает.
 * @param[in] action Действие пользователя, определённое в перечислении
 * UserAction_t
 * @param[in] hold Флаг удержания клавиши (true — клавиша удерживается, false —
 * нажата однократно)
 *
 * Преобразует пользовательское действие в соответствующее событие конечного
 * автомата (SnakeEvent) с помощью mapActionToEvent_(), затем передаёт событие
 * на обработку processEvent_().
 *
 * Поддерживаемые действия:
 * - UserAction_t::Start      → запуск новой игры (из состояний INIT или
 * GAME_OVER)
 * - UserAction_t::Pause      → переключение паузы (в состоянии MOVE)
 * - UserAction_t::Terminate  → немедленное завершение игры
 * - UserAction_t::Left/Right/Up/Down → попытка изменения направления движения
 *
 * @note Поведение зависит от текущего состояния FSM:
 *       - в состоянии PAUSED обрабатываются только Pause и Terminate
 *       - в состоянии GAME_OVER — только Start
 *       - в состоянии INIT — только Start
 * @note Флаг `hold` учитывается только для направлений — позволяет ускорить
 * движение или реализовать "скольжение", если поддерживается движком.
 * @note Некорректные действия (например, разворот на 180°) игнорируются на
 * уровне move_().
 * @note Безопасна для вызова из любого потока, но **не является
 * потокобезопасной**: одновременный вызов с update() или другим input() может
 * привести к гонкам.
 *
 * @warning Не вызывайте с указателем, полученным от create(), после destroy() —
 * UB.
 * @warning События могут игнорироваться, если не соответствуют текущему
 * состоянию.
 *
 * @par Пример:
 * @code
 * // Обработка ввода от пользователя
 * UserAction_t action = get_user_input();  // Например, из ncurses или GUI
 * bool is_held = is_key_held(action);
 * snake_handle_input(game, action, is_held);
 * @endcode
 */
void SnakeGame::handle_input(void* game, UserAction_t action,
                             bool hold) noexcept {
  (void)hold;
  if (!game) return;
  auto* self = static_cast<SnakeGame*>(game);
  auto event = self->mapActionToEvent_(action);
  if (event != SnakeEvent::NONE) {
    self->processEvent_(event);
  }
}

/**
 * @brief Обновляет состояние игры на один игровой тик
 * @param[in] game Указатель на экземпляр игры. Если равен nullptr, функция
 * ничего не делает.
 *
 * Выполняет основной игровой цикл:
 * - при состоянии MOVE: вызывает логику движения змейки (move_())
 * - при состоянии GAME_OVER: запускает таймер автосброса, осуществляет переход без события (SnakeGame::NONE)
 * - управляет переходами FSM через обработку событий
 * - обновляет игровые параметры: счёт, уровень, скорость, поле
 *
 * В процессе движения змейки автоматически выполняются:
 * - проверка столкновений с границами и телом
 * - съедание яблока и рост змейки
 * - генерация нового яблока
 * - увеличение уровня и ускорение каждые 10 очков
 * - обновление info_.field для отрисовки
 *
 * @note Должна вызываться регулярно (например, каждые 100–200 мс)
 *       для плавного геймплея.
 * @note При достижении длины SNAKE_MAX_LENGTH игра завершается с победой.
 * @note Безопасна для вызова с nullptr.
 * @note Потоконебезопасна — все вызовы должны происходить из одного потока.
 *
 * @warning Не вызывайте вручную события TICK — они обрабатываются
 * автоматически в движке.
 * @warning Избыточно частые вызовы ускорят игру, редкие — вызовут "лаги".
 * @warning Не используйте после destroy() — поведение не определено.
 *
 * @par Пример игрового цикла:
 * @code
 * while (is_running) {
 *     snake_update(game);                    // Логическое обновление
 *     const GameInfo_t* info = snake_get_info(game);
 *     render_game(info);                     // Отрисовка
 *     usleep(info->speed);                   // Задержка (в микросекундах)
 * }
 * @endcode
 *
 * @see move_() — основная логика движения и обработки событий
 * @see processEvent_() — обработка переходов FSM
 * @see GAME_OVER_DELAY_TICKS — количество тиков задержки перед сбросом
 */
void SnakeGame::update(void* game) noexcept {
  if (game == nullptr) return;
  SnakeGame* self = static_cast<SnakeGame*>(game);

  // Прямое обновление модели — только если игра активна
  if (self->getState() == SnakeState::MOVE) {
    self->move_();
  }

  // Обработка задержки после GAME_OVER
  if (self->getState() == SnakeState::GAME_OVER && !self->game_over_handled_) {
    self->game_over_timer_++;  // Увеличиваем таймер
    if (self->game_over_timer_ >= self->GAME_OVER_DELAY_TICKS) {
      self->processEvent_(SnakeEvent::AUTO_RESET);
      self->game_over_handled_ = true;
    }
  }
}

/**
 * @brief Получает текущее состояние игры для отображения
 * @param[in] game Константный указатель на экземпляр игры. Если равен nullptr,
 * возвращается nullptr.
 * @return Указатель на константную структуру GameInfo_t с актуальными данными
 * для рендеринга.
 *
 * Возвращает указатель на структуру, содержащую:
 * - игровое поле (info_.field) — матрица размером SNAKE_FIELD_ROWS ×
 * SNAKE_FIELD_COLS
 * - счёт (score), рекорд (high_score), уровень (level), скорость (speed, в
 * миллисекундах)
 * - флаги состояния: pause, game_over
 *
 * Перед возвратом вызывает updateFieldState_() для синхронизации игрового поля:
 * - ячейки с телом змейки помечаются как 1 (SNAKE_BODY_CELL)
 * - голова — как 2 (SNAKE_HEAD_CELL)
 * - яблоко — как 255 (SNAKE_APPLE_CELL)
 *
 * @note Возвращаемый указатель ссылается на внутренние данные экземпляра игры.
 * @note Указатель остаётся валидным **только до следующего вызова
 * snake_update() или snake_destroy()**. После этого он становится
 * недействительным (dangling pointer).
 * @note Безопасна для вызова с nullptr — возвращает nullptr.
 * @note Может вызываться из любого состояния (INIT, MOVE, PAUSED, GAME_OVER).
 * @note Потоконебезопасна: не вызывайте одновременно с другими функциями API
 * для одного экземпляра.
 *
 * @warning Не сохраняйте указатель надолго. Используйте его немедленно для
 * отрисовки.
 * @warning Не освобождайте память, на которую указывает возвращаемое значение —
 * она управляется классом.
 * @warning Не полагайтесь на стабильность адреса — он может меняться между
 * вызовами.
 *
 * @par Пример:
 * @code
 * const GameInfo_t* info = snake_get_info(game);
 * if (info) {
 *     draw_field(info->field);     // Отрисовка поля
 *     draw_ui(info->score, info->level, info->high_score);
 *     if (info->pause) {
 *         draw_pause_overlay();
 *     }
 * } else {
 *     fprintf(stderr, "Ошибка: недопустимый указатель игры\n");
 * }
 * @endcode
 */
const GameInfo_t* SnakeGame::get_info(const void* game) noexcept {
  if (game == nullptr) {
    return nullptr;
  }
  const SnakeGame* self = static_cast<const SnakeGame*>(game);
  // Возвращаем non-const через const_cast (безопасно, т.к. это наш объект)
  SnakeGame* mutable_self = const_cast<SnakeGame*>(self);
  mutable_self->updateFieldState_();
  return &self->info_;
}

/**
 * @private
 * @brief Приватный конструктор — инициализирует новую игровую сессию
 *
 * Полностью настраивает внутреннее состояние игры:
 * - инициализирует FSM в начальное состояние INIT
 * - выделяет память под info_.field (SNAKE_FIELD_ROWS × SNAKE_FIELD_COLS)
 * - выделяет память под info_.next (4×4, по требованию API)
 * - сбрасывает игровое состояние через initialize_()
 * - создаёт директорию для сохранений (если отсутствует)
 * - загружает рекорд из файла (SNAKE_SCORE_FILE)
 *
 * @note Вызывается **исключительно** через статический метод create() —
 *       прямое создание запрещено (конструктор копирования и перемещения удалены).
 * @note Все ресурсы (память, файлы) корректно освобождаются в деструкторе.
 * @note При ошибках выделения памяти или чтения файла:
 *       - объект всё равно создаётся
 *       - игра переходит в работоспособное состояние с дефолтными значениями
 *       - рекорд устанавливается в 0
 * @note Для безопасности и согласованности вся инициализация состояния
 *       делегирована методу initialize_().
 *
 * @warning Не вызывайте напрямую — используйте только snake_create() (C API).
 * @warning Не выбрасывает исключения: критические сбои (например, ошибка FSM)
 *          приводят к аварийному завершению (std::abort).
 *
 * @see initialize_()     — сброс игрового состояния
 * @see ensureScoreDir_() — создание директории сохранений
 * @see loadHighScore_()  — загрузка рекорда
 * @see ~SnakeGame()      — освобождение ресурсов
 */
SnakeGame::SnakeGame() noexcept {
  // Инициализация GameInfo_t
  std::memset(&info_, 0, sizeof(GameInfo_t));

  // Выделение памяти для игрового поля
  info_.field = new int*[SNAKE_FIELD_ROWS];
  for (int i = 0; i < SNAKE_FIELD_ROWS; ++i) {
    info_.field[i] = new int[SNAKE_FIELD_COLS];
    std::memset(info_.field[i], 0, SNAKE_FIELD_COLS * sizeof(int));
  }

  // Выделение памяти для next (не используется в Snake, но требуется API)
  info_.next = new int*[4];
  for (int i = 0; i < 4; ++i) {
    info_.next[i] = new int[4];
    std::memset(info_.next[i], 0, 4 * sizeof(int));
  }

  initialize_();

  ensureScoreDir_();
  loadHighScore_();

  // Инициализация игры

  bool ok = fsm_init(&fsm_, this, transitions_,
                     sizeof(transitions_) / sizeof(transitions_[0]),
                     to_fsm_state(SnakeState::INIT));

  if (!ok) {
    std::abort();
  }
}

/**
 * @private
 * @brief Приватный деструктор, освобождающий ресурсы игры
 *
 * Выполняет корректное завершение и очистку всех ресурсов, связанных с
 * экземпляром игры:
 * - сохраняет текущий рекорд (info_.high_score) в файл (SNAKE_SCORE_FILE)
 * - освобождает динамическую память, выделенную под:
 *   - игровое поле (info_.field) — двумерный массив размером SNAKE_FIELD_ROWS ×
 * SNAKE_FIELD_COLS
 *   - буфер next (info_.next) — дополнительный буфер, если используется
 * (например, для отображения следующего действия)
 * - уничтожает внутренние структуры: тело змейки (body_), множество занятых
 * ячеек (occupied_cells_)
 *
 * @note Деструктор вызывается **только** из статического метода destroy() —
 * прямое удаление запрещено.
 * @note Операция безопасна даже при частичной инициализации — проверяет
 * указатели на nullptr перед освобождением.
 * @note Сохранение рекорда выполняется всегда, если файл доступен для записи.
 * @note Помечен как noexcept — не выбрасывает исключения, что обязательно при
 * работе с C API.
 *
 * @warning Не вызывайте напрямую через delete — используйте только
 * snake_destroy() (C API).
 * @warning После завершения деструктора объект и все связанные данные
 * становятся недействительными.
 */
SnakeGame::~SnakeGame() noexcept {
  // Сохранение рекорда
  saveHighScore_();

  // Освобождение памяти game field
  if (info_.field != nullptr) {
    for (int i = 0; i < SNAKE_FIELD_ROWS; ++i) {
      delete[] info_.field[i];
    }
    delete[] info_.field;
  }

  // Освобождение памяти next
  if (info_.next != nullptr) {
    for (int i = 0; i < 4; ++i) {
      delete[] info_.next[i];
    }
    delete[] info_.next;
  }

  body_.clear();
}

/**
 * @private
 * @brief Полностью сбрасывает состояние игры к начальному
 *
 * Выполняет полную переинициализацию игровой сессии при старте новой игры
 * или перезапуске после GAME_OVER. Устанавливает все параметры в значения
 * по умолчанию:
 * - счёт (score = 0), уровень (level = 1), скорость (speed = 1)
 * - направление движения — вправо
 * - флаги: pause = 0, game_over_handled_ = false, таймер = 0
 * - очищает тело змейки (body_) и множество занятых ячеек (occupied_cells_)
 * - удаляет текущее яблоко (apple_x/y = -1)
 * - обнуляет игровое поле (info_.field)
 *
 * @note Не пересоздаёт память под info_.field и info_.next —
 *       используется уже выделённый буфер (инициализируется в конструкторе).
 * @note Является **единой точкой инициализации** — вызывается из:
 *       - конструктора
 *       - on_state_enter_ при переходе в INIT
 * @note После вызова необходимо отдельно инициализировать змейку и яблоко
 *       через initializeSnake_() и spawnApple_() — это делается в состоянии MOVE.
 * @note Безопасен для вызова в любом состоянии игры.
 *
 * @warning Не вызывайте напрямую из C API — используйте
 *          snake_handle_input(game, Start, false).
 * @warning Не изменяет рекорд (high_score) — он сохраняется между сессиями.
 * @warning Не размещает змейку и не генерирует яблоко — только подготавливает
 *          состояние для последующей инициализации.
 *
 * @see initializeSnake_() — размещение змейки после initialize_()
 * @see spawnApple_()      — генерация яблока
 * @see on_state_enter_()  — вызов при смене состояния FSM
 */
void SnakeGame::initialize_() noexcept {
  direction_ = Direction::RIGHT;
  next_direction_ = Direction::RIGHT;
  info_.score = 0;
  info_.level = 1;
  info_.speed = 1;
  info_.pause = 0;
  game_over_handled_ = false;
  game_over_timer_ = 0;
  apple_x_ = -1;
  apple_y_ = -1;
  body_.clear();
  occupied_cells_.clear();

  // Очистка поля
  for (int y = 0; y < SNAKE_FIELD_ROWS; ++y) {
    std::memset(info_.field[y], 0, SNAKE_FIELD_COLS * sizeof(int));
  }
}

/**
 * @private
 * @brief Инициализирует начальное положение змейки на поле
 *
 * Размещает змейку горизонтально в центре игрового поля, тянущейся влево от
 * центра. Голова змейки находится в центре поля, остальные сегменты добавляются
 * с шагом -1 по оси X.
 *
 * Параметры:
 * - длина: SNAKE_INITIAL_LENGTH (по умолчанию — 3)
 * - начальная позиция: (SNAKE_FIELD_COLS / 2, SNAKE_FIELD_ROWS / 2)
 * - направление: Direction::RIGHT (устанавливается в direction_ и
 * next_direction_)
 *
 * Пример размещения (при поле 10×20 и длине 3):
 *     (9,9) ← (8,9) ← (7,9)  [голова в (9,9)]
 *
 * @note Метод очищает текущее тело змейки (body_.clear()) перед инициализацией.
 * @note Вызывается только из initialize_() — не предназначен для прямого
 * вызова.
 * @note Координаты проверяются на выход за границы — гарантируется валидность
 * позиции.
 * @note После размещения обновляется множество занятых ячеек
 * (updateOccupiedCells_).
 *
 * @warning Убедитесь, что SNAKE_INITIAL_LENGTH ≤ SNAKE_FIELD_COLS, иначе змейка
 * может выйти за левую границу.
 * @warning Не меняет состояние FSM, счёт, уровень или позицию яблока — только
 * тело и направление.
 */
void SnakeGame::initializeSnake_() noexcept {
    // Исправляем расчет центральной позиции
    int start_x = SNAKE_FIELD_COLS / 2;
    int start_y = SNAKE_FIELD_ROWS / 2;

    body_.clear();
    
    // Создаем змейку, тянущуюся влево от центра
    for (int i = 0; i < SNAKE_INITIAL_LENGTH; ++i) {
        body_.push_back(SnakeSegment(start_x - i, start_y));
    }
    
    direction_ = Direction::RIGHT;
    next_direction_ = Direction::RIGHT;
    
    // Полная перестройка множества занятых ячеек
    rebuildOccupiedCells_();
}

/**
 * @private
 * @brief Обеспечивает существование директории для сохранения рекордов
 *
 * Проверяет, существует ли директория, указанная в SNAKE_SCORE_DIR.
 * Если директория отсутствует, создаёт её с правами доступа 0755 (rwxr-xr-x).
 *
 * Используется при создании экземпляра игры для подготовки места хранения
 * файла рекорда (SNAKE_SCORE_FILE). Гарантирует, что последующие операции
 * записи не завершатся ошибкой из-за отсутствия пути.
 *
 * @note Выполняется только один раз при инициализации игры.
 * @note Безопасна при повторных вызовах — не пытается пересоздать существующую
 * директорию.
 * @note Использует POSIX-совместимые функции stat() и mkdir() — работает на
 * Unix-подобных системах.
 * @note На Windows может требовать адаптации (в текущей реализации используется
 * MinGW или WSL).
 * @note Не выбрасывает исключения — все ошибки игнорируются (кроме отладки).
 *
 * @warning Если процесс не имеет прав на запись в родительскую директорию,
 *          создание каталога может не удаться, и сохранение рекорда будет
 * отключено.
 *
 * @par Пример пути:
 * - Linux/macOS: ~/.brickgame
 * - Windows (MSYS2/MinGW): %USERPROFILE%/.brickgame
 */
void SnakeGame::ensureScoreDir_() noexcept {
  struct stat st = {};
  if (stat(SNAKE_SCORE_DIR, &st) == -1) {
    mkdir(SNAKE_SCORE_DIR, 0755);
  }
}

/**
 * @private
 * @brief Загружает рекорд из файла на диске
 * @return true в случае успеха, false при ошибке (например, файл не существует,
 * повреждён или недоступен)
 *
 * Пытается открыть файл, путь к которому определён как SNAKE_SCORE_FILE, и
 * прочитать из него целочисленное значение — сохранённый рекорд (high_score).
 * Значение сохраняется в info_.high_score.
 *
 * Поведение при ошибках:
 * - если файл не существует — считается, что рекорд не установлен;
 * info_.high_score = 0
 * - если файл пуст или содержит нечисловые данные — info_.high_score = 0
 * - если нет прав на чтение — info_.high_score = 0
 *
 * @note Вызывается один раз при создании экземпляра игры (в конструкторе).
 * @note Метод идемпотентен: повторный вызов перечитывает файл и может обновить
 * рекорд.
 * @note Использует потоковую обработку (std::ifstream) — безопасен с точки
 * зрения C++.
 * @note Не выбрасывает исключения — все ошибки обрабатываются внутренне.
 *
 * @warning Не ожидается, что файл будет изменяться извне во время работы игры.
 *          Если это происходит, новое значение будет загружено только при
 * перезапуске.
 * @warning Убедитесь, что директория ~/.brickgame (или эквивалент) создана — в
 * противном случае файл не будет найден, даже если создан (см.
 * ensureScoreDir_()).
 */
bool SnakeGame::loadHighScore_() noexcept {
  std::ifstream file(SNAKE_SCORE_FILE);
  if (file.is_open()) {
    file >> info_.high_score;
    file.close();
    return true;
  }
  info_.high_score = 0;
  return false;
}

/**
 * @private
 * @brief Сохраняет текущий рекорд в файл
 * @return true в случае успеха записи, иначе false
 *
 * Записывает значение `info_.high_score` в файл, путь к которому определён как
 * SNAKE_SCORE_FILE. Перед записью проверяет, изменилось ли значение рекорда по
 * сравнению с загруженным — если нет, операция может быть пропущена
 * (оптимизация на будущее).
 *
 * Файл открывается в режиме записи (std::ofstream), что приводит к полной
 * перезаписи содержимого. Обеспечивает атомарность записи: сначала запись во
 * временный файл, затем переименование в целевое имя. Это предотвращает
 * повреждение данных при сбое во время сохранения (например, прерывание power
 * loss).
 *
 * @note Используется:
 *       - при уничтожении экземпляра игры (в деструкторе)
 *       - при обновлении рекорда во время игры (опционально)
 * @note Метод потоконебезопасен — не вызывайте одновременно из нескольких
 * потоков.
 * @note Не выбрасывает исключения — все ошибки обрабатываются внутренне.
 *
 * @warning Операция может завершиться неудачей, если:
 *          - нет прав на запись в директорию SNAKE_SCORE_DIR
 *          - диск переполнен
 *          - файл заблокирован другим процессом
 * @warning Не гарантирует синхронизацию с диском (fsync), но использует
 * flush().
 *
 * @par Алгоритм:
 *   1. Создать уникальное имя временного файла в директории сохранений
 *   2. Открыть временный файл и записать туда info_.high_score
 *   3. Закрыть файл
 *   4. Переименовать временный файл в SNAKE_SCORE_FILE
 *   5. Если на любом шаге ошибка — вернуть false
 */
bool SnakeGame::saveHighScore_() const noexcept {
  std::ofstream file(SNAKE_SCORE_FILE, std::ios::out | std::ios::trunc);
  if (!file.is_open()) return false;

  file << info_.high_score << '\n';
  return static_cast<bool>(file);
}

/**
 * @private
 * @brief Преобразует действие пользователя в событие конечного автомата (FSM)
 * @param action Действие пользователя, определённое в перечислении UserAction_t
 * @return Соответствующее событие из перечисления SnakeEvent; SnakeEvent::NONE,
 * если действие не распознано
 *
 * Выполняет маппинг действий от C API (UserAction_t) в события игры
 * (SnakeEvent), используемые для управления FSM. Поддерживаемые преобразования:
 * - UserAction_t::Start      → SnakeEvent::START
 * - UserAction_t::Pause      → SnakeEvent::PAUSE_TOGGLE
 * - UserAction_t::Terminate  → SnakeEvent::TERMINATE
 * - UserAction_t::Left       → SnakeEvent::MOVE_LEFT
 * - UserAction_t::Right      → SnakeEvent::MOVE_RIGHT
 * - UserAction_t::Up         → SnakeEvent::MOVE_UP
 * - UserAction_t::Down       → SnakeEvent::MOVE_DOWN
 * - Любое другое значение    → SnakeEvent::NONE
 *
 * @note Метод не зависит от текущего состояния игры — маппинг статичен.
 * @note Используется исключительно в handle_input() — не предназначен для
 * прямого вызова вне класса.
 * @note Удержание клавиши (hold) не влияет на результат — обрабатывается на
 * уровне update().
 *
 * @warning Не добавляйте новые действия без обновления этой таблицы — иначе они
 * будут игнорироваться.
 * @warning Не используйте switch с отсутствием default — гарантируется
 * обработка всех неизвестных значений.
 */
SnakeEvent SnakeGame::mapActionToEvent_(UserAction_t action) const noexcept {
  switch (action) {
    case Start:
      return SnakeEvent::START;
    case Pause:
      return SnakeEvent::PAUSE_TOGGLE;
    case Terminate:
      return SnakeEvent::TERMINATE;
    case Left:
      return SnakeEvent::MOVE_LEFT;
    case Right:
      return SnakeEvent::MOVE_RIGHT;
    case Up:
      return SnakeEvent::MOVE_UP;
    case Down:
      return SnakeEvent::MOVE_DOWN;
    case Action:
      return SnakeEvent::NONE;
    default:
      return SnakeEvent::NONE;
  }
}

/**
 * @private
 * @brief Обрабатывает событие конечного автомата (FSM)
 * @param ev Событие типа SnakeEvent, которое необходимо обработать
 *
 * Централизованная точка диспетчеризации событий. В зависимости от текущего
 * состояния игры и поступившего события вызывает соответствующую логику:
 * - START        → переход в состояние INIT или MOVE
 * - PAUSE_TOGGLE → переключение между MOVE и PAUSED
 * - TERMINATE    → завершение игры (GAME_OVER)
 * - MOVE_*       → обновление направления движения
 * - TICK         → выполнение одного шага игры (движение змейки)
 *
 * Если событие равно SnakeEvent::NONE, обработка прекращается.
 *
 * @note Метод использует таблицу переходов FSM, но также включает
 * дополнительную логику, недоступную на чистом C уровне (например, вызов
 * move_()).
 * @note Вызывается только из on_state_enter_() — не предназначен для прямого
 * вызова.
 * @note Безопасен при неизвестных комбинациях "состояние + событие" —
 * игнорирует недопустимые переходы.
 * @note Потоконебезопасен — все вызовы должны происходить из одного потока.
 *
 * @warning Не добавляйте новую логику обработки событий вне этого метода —
 * нарушится единая точка управления.
 * @warning Не изменяйте состояние напрямую — используйте processEvent_() для
 * согласованности.
 */
void SnakeGame::processEvent_(SnakeEvent ev) noexcept {
  if (ev == SnakeEvent::NONE) return;

  switch (ev) {
      case SnakeEvent::MOVE_LEFT:
        next_direction_ = Direction::LEFT;
        return;
      case SnakeEvent::MOVE_RIGHT:
        next_direction_ = Direction::RIGHT;
        return;
      case SnakeEvent::MOVE_UP:
        next_direction_ = Direction::UP;
        return;
      case SnakeEvent::MOVE_DOWN:
        next_direction_ = Direction::DOWN;
        return;
      default:
        fsm_process_event(&fsm_, to_fsm_event(ev));
        break;
    }
}

/**
 * @private
 * @brief Выполняет одно игровое движение змейки
 *
 * Обновляет состояние игры на один тик в состоянии MOVE. Выполняет следующие
 * шаги:
 * 1. Обновление направления движения (если введено корректное — без разворота
 * на 180°)
 * 2. Вычисление новой позиции головы на основе текущего направления
 * 3. Проверка столкновения с границами игрового поля и сегментами тела
 * 4. При отсутствии столкновения:
 *    - перемещение головы на новую позицию (вставка в начало body_)
 *    - проверка, находится ли голова на яблоке
 *    - если яблоко съедено: вызов eatApple_(), хвост не удаляется
 *    - если не съедено: удаление последнего сегмента (хвоста)
 * 5. Обновление множества занятых ячеек (occupied_cells_)
 * 6. Обновление игрового поля (info_.field) через updateFieldState_()
 * 7. Проверка условия победы — если вся доступная площадь поля занята змейкой
 *
 * @note Метод вызывается при событии TICK в состоянии MOVE.
 * @note Направление движения берётся из next_direction_, что позволяет
 * буферизовать ввод.
 * @note Победа (полное заполнение поля) обрабатывается как GAME_OVER с
 * выигрышем.
 * @note Не изменяет состояние FSM напрямую — при столкновении вызывает
 * handleCollision_().
 *
 * @warning Не вызывайте напрямую вне processEvent_() — нарушится FSM-логика.
 * @warning Не меняйте тело змейки (body_) вручную — нарушится целостность
 * данных.
 */
void SnakeGame::move_() noexcept {
  if (body_.empty()) {
    return;
  }

  // Проверяем, не является ли попытка разворотом на 180 градусов
  if (!((next_direction_ == Direction::UP && direction_ == Direction::DOWN) ||
        (next_direction_ == Direction::DOWN && direction_ == Direction::UP) ||
        (next_direction_ == Direction::LEFT && direction_ == Direction::RIGHT) ||
        (next_direction_ == Direction::RIGHT && direction_ == Direction::LEFT))) {
    direction_ = next_direction_;  // меняем направление
  }
  // Если изменение невозможно — оставляем старое направление


  // Получаем голову (первый элемент)
  SnakeSegment head = body_.front();

  // Движение в зависимости от направления
  switch (direction_) {
    case Direction::UP:
      head.y--;
      break;
    case Direction::DOWN:
      head.y++;
      break;
    case Direction::LEFT:
      head.x--;
      break;
    case Direction::RIGHT:
      head.x++;
      break;
  }

  // Проверка столкновения с границей
  if (checkCollision_(head)) {
    handleCollision_();
    return;
  }

  // Проверяем, съели ли яблоко (до вставки!)
  bool ate = checkAppleEaten_(head);

  body_.push_front(head);

  if (!ate) {
    body_.pop_back();
  } else {
    eatApple_();
    updateScore_();
  }

  updateOccupiedCells_();

  // проверка победы
  if (body_.size() >= SNAKE_MAX_LENGTH) {
    if (info_.score > info_.high_score) {
      info_.high_score = info_.score;
      saveHighScore_();
    }
    processEvent_(SnakeEvent::TERMINATE);
    return;
  }
}

/**
 * @private
 * @brief Проверяет столкновение головы змейки с границами поля или её телом
 * @param head Сегмент с координатами новой головы (после следующего шага)
 * @return true, если столкновение обнаружено (с границей или телом), иначе
 * false
 *
 * Выполняет две проверки:
 * 1. **Выход за границы игрового поля**:
 *    - x ∈ [0; SNAKE_FIELD_COLS - 1]
 *    - y ∈ [0; SNAKE_FIELD_ROWS - 1]
 * 2. **Пересечение с телом змейки**:
 *    - перебирает все сегменты из body_, начиная со второго (после головы)
 *    - хвост (последний элемент) исключается из проверки, так как при движении
 * он будет удалён
 *    - используется множество occupied_cells_ для O(1) проверки (если включено)
 *
 * @note Метод вызывается перед фактическим перемещением — head ещё не добавлен
 * в body_.
 * @note Использует быструю проверку через std::unordered_set<int>
 * occupied_cells_, ключ = y * SNAKE_FIELD_COLS + x.
 * @note Не изменяет состояние игры — чистая функция.
 * @note Безопасен при пустом теле (body_.size() == 1) — не будет проверки на
 * само-столкновение.
 *
 * @warning Не проверяет столкновение с яблоком — это делает отдельный метод
 * checkAppleEaten_().
 * @warning Не вызывайте с невалидными координатами — поведение не определено.
 */
bool SnakeGame::checkCollision_(const SnakeSegment& head) const noexcept {
  // Проверка границ
  if (head.x < 0 || head.x >= SNAKE_FIELD_COLS || head.y < 0 ||
      head.y >= SNAKE_FIELD_ROWS) {
    return true;
  }

  // Проверка столкновения с телом (не включая хвост, который исчезнет)
  // Начинаем с 1, так как 0 — это новая голова (она же head)
  for (size_t i = 1; i < body_.size(); ++i) {
    if (body_[i].x == head.x && body_[i].y == head.y) {
      return true;
    }
  }

  return false;
}

/**
 * @private
 * @brief Выполняет действия при столкновении головы змейки с границей или телом
 *
 * Обрабатывает финальную логику при обнаружении столкновения:
 * - переводит игру в состояние GAME_OVER через отправку события TERMINATE
 * - гарантирует корректное обновление состояния FSM
 * - позволяет обновить рекорд, если текущий счёт превысил предыдущий
 *
 * Метод не изменяет состояние напрямую — использует существующий механизм
 * событий (processEvent_), что обеспечивает согласованность с логикой конечного
 * автомата.
 *
 * @note Вызывается из move_() сразу после обнаружения столкновения.
 * @note Не выполняет немедленное обновление поля или сохранение — делегирует
 * это FSM.
 * @note Является точкой завершения активной игровой сессии.
 *
 * @warning Не вызывайте при длине змейки 1 — логически невозможно столкнуться с
 * собой.
 * @warning Не меняйте состояние вручную — используйте processEvent_() для
 * согласованности.
 *
 * @par Пример:
 * @code
 * if (checkCollision_(new_head)) {
 *     handleCollision_();  // Гарантирует переход в GAME_OVER
 *     return;
 * }
 * @endcode
 */
void SnakeGame::handleCollision_() noexcept {
  processEvent_(SnakeEvent::TERMINATE);  // → MOVE → GAME_OVER
}

/**
 * @private
 * @brief Проверяет, находится ли голова змейки на яблоке
 * @param head Сегмент с координатами головы (после следующего шага движения)
 * @return true, если координаты головы совпадают с координатами яблока; иначе
 * false
 *
 * Сравнивает координаты переданного сегмента (новой головы) с текущими
 * координатами яблока (apple_x_, apple_y_). Используется в методе move_() сразу
 * после вычисления новой позиции головы.
 *
 * @note Метод вызывается **до** фактического добавления головы в тело (body_).
 * @note Не изменяет состояние игры — чистая функция.
 * @note Является частью логики съедания яблока: при возврате true вызывается
 * eatApple_().
 * @note Если возвращается true, хвост змейки не удаляется при следующем
 * движении (змейка растёт).
 *
 * @warning Не выполняет проверку выхода за границы — это делает
 * checkCollision_().
 * @warning Не сбрасывает позицию яблока — сброс выполняется в eatApple_().
 *
 * @par Пример:
 * @code
 * if (checkAppleEaten_(new_head)) {
 *     eatApple_();        // Увеличивает счёт, генерирует новое яблоко
 *     // Хвост не удаляется — змейка растёт на один сегмент
 * } else {
 *     body_.pop_back();   // Обычное движение — хвост удаляется
 * }
 * @endcode
 */
bool SnakeGame::checkAppleEaten_(const SnakeSegment& head) const noexcept {
  return head.x == apple_x_ && head.y == apple_y_;
}

/**
 * @private
 * @brief Выполняет полную логику съедания яблока
 *
 * Вызывается из move_() при обнаружении, что голова змейки пересекает позицию
 * яблока. Выполняет следующие действия:
 * 1. Увеличивает текущий счёт (info_.score) на 10 очков
 * 2. Проверяет, превышен ли рекорд (info_.score > info_.high_score):
 *    - если да — обновляет рекорд
 *    - при необходимости инициирует сохранение в файл (saveHighScore_())
 * 3. Вызывает updateScore_() для проверки:
 *    - повышения уровня каждые 10 очков
 *    - увеличения скорости (уменьшения задержки)
 * 4. Генерирует новое яблоко на свободной ячейке поля (spawnApple_())
 *
 * @note Метод не изменяет тело змейки напрямую — рост достигается за счёт
 *       удержания хвоста в move_() (не вызывается pop_back()).
 * @note Вызывается только из move_() — не предназначен для прямого вызова.
 * @note Потоконебезопасен — все изменения происходят в основном игровом потоке.
 * @note Не сбрасывает флаги состояния (pause, game_over) — только обновляет
 * данные.
 *
 * @warning Не вызывайте вручную с невалидным состоянием — может привести к
 * некорректному счёту.
 * @warning Не дублируйте вызов — это приведёт к ложному увеличению счёта и
 * лишнему яблоку.
 *
 * @par Пример логики:
 * @code
 * info_.score += 1;
 * if (info_.score > info_.high_score) {
 *     info_.high_score = info_.score;
 *     saveHighScore_();  // Асинхронное сохранение
 * }
 * updateScore_();        // Проверка уровня и скорости
 * spawnApple_();         // Новое яблоко
 * @endcode
 */
void SnakeGame::eatApple_() noexcept {
  // Увеличиваем счёт
  info_.score += 1;

  // Обновляем рекорд
  if (info_.score > info_.high_score) {
    info_.high_score = info_.score;
  }

  // Генерируем новое яблоко
  spawnApple_();
}

/**
 * @private
 * @brief Генерирует новую позицию для яблока
 *
 * Выполняет безопасную генерацию яблока на свободной ячейке игрового поля:
 * 1. Формирует список всех пустых ячеек (не занятых телом змейки) через перебор
 * поля или анализ множества occupied_cells_.
 * 2. Если свободные ячейки есть:
 *    - выбирает одну случайную ячейку с использованием равномерного
 * распределения
 *    - устанавливает координаты яблока (apple_x_, apple_y_)
 * 3. Если свободных ячеек нет (все поле занято — теоретически возможно при
 * максимальной длине):
 *    - размещает яблоко на хвосте змейки (последний сегмент body_.back())
 *    - это гарантирует, что яблоко всегда появится, а следующим ходом хвост
 * уйдёт
 *
 * Использует thread_local генератор std::mt19937 с инициализацией по времени
 * для равномерного и непредсказуемого распределения.
 *
 * @note Вызывается из eatApple_() и initialize_() — каждый раз при
 * необходимости нового яблока.
 * @note Гарантирует валидность координат: яблоко всегда находится в пределах
 * поля.
 * @note Потоконебезопасен — использует thread_local генератор.
 * @note Не вызывает updateFieldState_() — обновление поля выполняется отдельно.
 *
 * @warning При ручном изменении тела змейки (body_) без вызова
 * updateOccupiedCells_() может сгенерировать яблоко в занятой ячейке.
 * @warning Не проверяет, совпадает ли новая позиция с текущей — возможен
 * "безопасный" дубль.
 *
 * @par Алгоритм:
 *   1. Собрать список свободных ячеек: все (x, y), где !(x,y) в occupied_cells_
 *   2. Если список не пуст — выбрать случайный элемент через
 * std::uniform_int_distribution
 *   3. Иначе — использовать позицию хвоста (body_.back())
 *   4. Установить apple_x_, apple_y_
 */
void SnakeGame::spawnApple_() noexcept {
  std::vector<SnakeSegment> free_cells;
  free_cells.reserve(SNAKE_FIELD_ROWS * SNAKE_FIELD_COLS - body_.size());

  for (int y = 0; y < SNAKE_FIELD_ROWS; ++y) {
    for (int x = 0; x < SNAKE_FIELD_COLS; ++x) {
      if (occupied_cells_.find(y * SNAKE_FIELD_COLS + x) ==
          occupied_cells_.end()) {
        free_cells.emplace_back(x, y);
      }
    }
  }

  // Генерируем случайный индекс с помощью C++11+ random
  thread_local std::mt19937 gen([]() {
    std::random_device rd;
    // Если random_device слабый (например, MinGW), добавляем время как резерв
    return rd.entropy() != 0
               ? rd()
               : static_cast<unsigned>(std::time(nullptr) ^
                                       (std::hash<std::thread::id>{}(
                                           std::this_thread::get_id())));
  }());

  if (!free_cells.empty()) {
    std::uniform_int_distribution<size_t> dist(0, free_cells.size() - 1);
    size_t index = dist(gen);
    apple_x_ = free_cells[index].x;
    apple_y_ = free_cells[index].y;
  } else if (!body_.empty()) {
    // Если нет свободных ячеек, размещаем яблоко на хвосте змеи
    // Следующим ходом хвост уйдет, и яблоко окажется в свободной ячейке
    const auto& tail = body_.back();
    apple_x_ = tail.x;
    apple_y_ = tail.y;
  } else {
    // Крайний случай: тело пустое (маловероятно, но для безопасности)
    apple_x_ = SNAKE_FIELD_COLS / 2;
    apple_y_ = SNAKE_FIELD_ROWS / 2;
  }
}

/**
 * @private
 * @brief Обновляет множество занятых ячеек игрового поля инкрементально
 *
 * Метод обновляет состояние множества занятых ячеек, отражающее текущее
 * положение змейки на игровом поле. Вместо полной перестройки множества (что
 * имело бы сложность O(n)), применяется инкрементальный подход с минимальными
 * изменениями (сложность O(1)).
 *
 * Алгоритм работы:
 * 1. Если змейка не выросла (не съела яблоко), удаляет из множества координаты
 * хвостового сегмента
 * 2. Всегда добавляет в множество координаты нового головного сегмента
 * 3. Промежуточные сегменты не обрабатываются - они остаются на своих местах
 *
 * @details Метод обеспечивает высокую производительность за счет:
 * - Избежания полного перебора всех сегментов тела змейки
 * - Использования хэш-таблицы (std::unordered_set) для операций вставки и
 * удаления за O(1)
 * - Минимизации количества операций - только добавление головы и удаление
 * хвоста при необходимости
 *
 * @pre Тело змейки (body_) должно содержать валидные координаты сегментов
 * @pre Координаты всех сегментов должны находиться в пределах игрового поля
 * @post Множество occupied_cells_ содержит актуальные координаты всех занятых
 * ячеек
 *
 * @note Метод безопасен для вызова с пустым телом змейки
 * @note Метод корректно работает при любом размере тела змейки
 * @note Не требует дополнительной памяти для выполнения
 * @note Гарантирует согласованность состояния между body_ и occupied_cells_
 *
 * @warning Не вызывайте метод при поврежденном состоянии body_ - это может
 * привести к некорректному состоянию множества occupied_cells_
 * @warning Не изменяйте body_ во время выполнения метода - это может вызвать
 * race condition
 */
void SnakeGame::updateOccupiedCells_() noexcept {
  // Удаление хвоста (если змейка не выросла)
  // Происходит только если длина изменилась и есть хотя бы 2 сегмента
  if (body_.size() > 1) {
    const auto& tail = body_.back();
    int tail_key = tail.y * SNAKE_FIELD_COLS + tail.x;
    occupied_cells_.erase(tail_key);
  }

  // Добавление новой головы (всегда происходит)
  const auto& head = body_.front();
  int head_key = head.y * SNAKE_FIELD_COLS + head.x;
  occupied_cells_.insert(head_key);
}

/**
 * @brief Полностью перестраивает множество занятых ячеек игрового поля
 *
 * Метод выполняет полную пересборку множества занятых ячеек на основе текущего
 * положения всех сегментов тела змейки. Используется для восстановления
 * согласованного состояния множества occupied_cells_ с телом змейки, например,
 * после сложных изменений или при инициализации.
 *
 * @details Алгоритм работы:
 * 1. Очищает текущее множество занятых ячеек
 * 2. Резервирует необходимую память для оптимизации вставки
 * 3. Перебирает все сегменты тела змейки и добавляет их координаты в множество
 *
 * @note Сложность алгоритма: O(n), где n — количество сегментов в теле змейки
 * @note Использование reserve() уменьшает количество перераспределений памяти
 * @note Статическое приведение к int обеспечивает предсказуемость и избегает
 * предупреждений
 * @note Координаты преобразуются в уникальный ключ: y * SNAKE_FIELD_COLS + x
 *
 * @pre Тело змейки (body_) должно содержать валидные координаты сегментов
 * @pre Координаты всех сегментов должны находиться в пределах игрового поля
 * @post Множество occupied_cells_ содержит все занятые ячейки без дубликатов
 * @post Размер occupied_cells_ равен количеству уникальных сегментов в body_
 *
 * @warning Не используйте метод в критически важных по времени участках кода
 *          (например, в основном игровом цикле) — предпочтительнее
 * updateOccupiedCells_()
 * @warning Вызывает полный перебор всех сегментов — может быть медленным при
 * длинной змейке
 * @warning Очищает текущее состояние множества — все предыдущие данные теряются
 *
 * @see updateOccupiedCells_() — более производительный инкрементальный метод
 * обновления
 */
void SnakeGame::rebuildOccupiedCells_() noexcept {
  occupied_cells_.clear();
  occupied_cells_.reserve(body_.size());

  for (const auto& seg : body_) {
    int key = static_cast<int>(seg.y * SNAKE_FIELD_COLS + seg.x);
    occupied_cells_.insert(key);
  }
}

/**
 * @brief Обновляет внутреннее представление игрового поля для отрисовки
 *
 * Преобразует текущее состояние игры (тело змейки, позиция головы, яблоко)
 * в двумерный массив info_.field, используемый движком рендеринга.
 * Каждая ячейка поля помечается значением:
 * - 0: пустая ячейка
 * - 1: сегмент тела змейки (SNAKE_BODY_CELL)
 * - 2: голова змейки (SNAKE_HEAD_CELL)
 * - 3: яблоко (SNAKE_APPLE_CELL, выделено отдельным значением для удобства)
 *
 * Алгоритм:
 * 1. Заполняет всё поле нулями (очистка предыдущего состояния)
 * 2. Добавляет яблоко на позицию (apple_x_, apple_y_)
 * 3. Добавляет сегменты змейки:
 *    - все сегменты, кроме первого — как тело (1)
 *    - первый сегмент (голова) — как голова (2)
 *
 * @note Вызывается перед каждым получением состояния через get_info().
 * @note Метод не выполняет отрисовку — только обновляет данные.
 * @note Гарантирует актуальность info_.field до следующего изменения состояния.
 * @note Потоконебезопасен, если вызывается из того же потока, что и update().
 *
 * @warning Не изменяйте info_.field напрямую — все изменения должны проходить
 * через этот метод.
 * @warning Не пропускайте вызов — иначе на экране будет устаревшее состояние.
 *
 * @par Пример использования (внутри get_info()):
 * @code
 * updateFieldState_();  // Подготавливаем данные
 * return &info_;        // Возвращаем указатель для рендеринга
 * @endcode
 */
void SnakeGame::updateFieldState_() noexcept {
    // Гарантированная очистка поля
    for (int y = 0; y < SNAKE_FIELD_ROWS; ++y) {
        for (int x = 0; x < SNAKE_FIELD_COLS; ++x) {
            info_.field[y][x] = 0;
        }
    }

    // Отрисовка яблока
    if (apple_y_ >= 0 && apple_y_ < SNAKE_FIELD_ROWS && 
        apple_x_ >= 0 && apple_x_ < SNAKE_FIELD_COLS) {
        info_.field[apple_y_][apple_x_] = SNAKE_APPLE_CELL;
    }

    // Отрисовка змейки
    if (!body_.empty()) {
        // Голова (первый элемент)
        const auto& head = body_.front();
        if (head.y >= 0 && head.y < SNAKE_FIELD_ROWS && 
            head.x >= 0 && head.x < SNAKE_FIELD_COLS) {
            info_.field[head.y][head.x] = SNAKE_HEAD_CELL;
        }
        
        // Тело (остальные элементы)
        for (size_t i = 1; i < body_.size(); ++i) {
            const auto& segment = body_[i];
            if (segment.y >= 0 && segment.y < SNAKE_FIELD_ROWS && 
                segment.x >= 0 && segment.x < SNAKE_FIELD_COLS) {
                info_.field[segment.y][segment.x] = SNAKE_BODY_CELL;
            }
        }
    }
}

/**
 * @brief Обновляет счёт, уровень и скорость игры
 *
 * Выполняет динамическую настройку игровой сложности на основе текущего счёта:
 * 1. Обновляет текущий уровень: level = (score / 10) + 1
 * 2. Вычисляет новую скорость (задержку между тиками) по формуле:
 *    - base_speed - (level - 1) * speed_step
 *    - где base_speed — начальная скорость (например, 200 мс), speed_step — шаг
 * ускорения (например, 10 мс)
 * 3. Ограничивает минимальную скорость (info_.speed) значением SNAKE_MIN_SPEED
 * (по умолчанию — 50 мс)
 *
 * Пример:
 * - При score = 0  → level 1, speed = 200 мс
 * - При score = 10 → level 2, speed = 190 мс
 * - При score = 90 → level 10, speed = 110 мс
 * - При score = 100 → level 11, speed = 50 мс (ограничение)
 *
 * @note Вызывается каждый раз при увеличении счёта (в eatApple_()).
 * @note Уровень и скорость зависят только от счёта — детерминированное
 * поведение.
 * @note Не влияет на рекорд (high_score) — он обновляется отдельно.
 * @note Потоконебезопасен — вызывается из основного игрового потока.
 *
 * @warning Не вызывайте при ручном изменении info_.score без обновления —
 * уровень и скорость не синхронизируются.
 * @warning Не изменяйте info_.speed напрямую — будет проигнорировано при
 * следующем обновлении.
 *
 * @par Пример использования:
 * @code
 * info_.score += 10;
 * updateScore_();  // Автоматически повысит уровень и ускорит игру при
 * необходимости
 * @endcode
 */
void SnakeGame::updateScore_() noexcept {
  // Обновление уровня в зависимости от счёта
  info_.level = 1 + (info_.score / 10);
  // Увеличение скорости с уровнем
  info_.speed = 100 - (info_.level * 5);
  if (info_.speed < 50) {
    info_.speed = 50;
  }
}

/**
 * @brief Колбэк, вызываемый при входе в новое состояние конечного автомата
 * @param ctx Указатель на контекст FSM (экземпляр SnakeGame)
 *
 * Выполняет синхронизацию внутреннего состояния игры с текущим состоянием FSM.
 * Отвечает за:
 * - сброс или инициализацию логики игры при входе в состояние
 * - обновление флагов отображения (например, info_.pause)
 * - подготовку игровых структур (змейка, яблоко) при необходимости
 *
 * Обработка состояний:
 * - INIT:     полный сброс игры через initialize_() — подготовка к новой сессии
 * - MOVE:     снимает паузу; при пустом теле — инициализирует змейку и яблоко
 * - PAUSED:   устанавливает флаг pause = 1 — интерфейс отображает паузу
 * - GAME_OVER: снимает паузу (pause = 0), сохраняя счёт для отображения
 *
 * @note Является статическим колбэком для C-совместимого FSM — принимает void*.
 * @note Вызывается автоматически при каждом переходе состояния.
 * @note Централизует инициализацию: все сбросы проходят через initialize_().
 * @note Не обрабатывает логику движения — только входные и UI-эффекты.
 * @note Не выбрасывает исключения — noexcept (критично для C-интерфейса).
 *
 * @warning Не вызывайте напрямую — только через FSM.
 * @warning Контекст (ctx) должен указывать на валидный экземпляр SnakeGame.
 * @warning Не меняйте логику инициализации без синхронизации с FSM и C API.
 *
 * @par Особенность:
 * Инициализация змейки и яблока происходит **только при первом входе в MOVE**
 * (проверка через body_.empty()). Это предотвращает пересоздание при паузе/продолжении.
 *
 * @see initialize_()       — сброс игрового состояния
 * @see initializeSnake_()  — размещение змейки в центре поля
 * @see spawnApple_()       — генерация яблока на свободной ячейке
 * @see rebuildOccupiedCells_() — обновление множества занятых позиций
 */
void SnakeGame::on_state_enter_(fsm_context_t ctx) {
  auto* self = static_cast<SnakeGame*>(ctx);
  SnakeState new_state = from_fsm_state(self->fsm_.current);

  switch (new_state) {
    case SnakeState::INIT:
      self->initialize_();
      break;

    case SnakeState::MOVE:
      self->info_.pause = 0;
      
      // Инициализация только если тело пустое
      if (self->body_.empty()) {
        self->initializeSnake_();
        self->rebuildOccupiedCells_();
        self->spawnApple_();
      }
      break;

    case SnakeState::PAUSED:
      self->info_.pause = 1;
      break;

    case SnakeState::GAME_OVER:
      self->info_.pause = 0;
      break;

    default:
      break;
  }
}

}  // namespace s21