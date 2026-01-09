/**
 * @file s21_bgame.c
 * @brief Реализация игрового фреймворка: реестр игр, переключение, ввод и состояние
 *
 * Содержит реализацию глобального реестра игр, контекста активной игры и обёрток
 * для унифицированного доступа к методам через userInput() и updateCurrentState().
 * Управляет жизненным циклом экземпляров игр: создание, уничтожение, переключение.
 *
 * Включает ленивую инициализацию, защиту от дублирования, проверку целостности
 * интерфейса игры. Предназначен для интеграции с любыми играми, реализующими
 * GameInterface_t.
 *
 * @author provemet
 * @date 2025
 * @version 1.0
 *
 * @see s21_bgame.h
 * @note Функции bg_* не являются потокобезопасными.
 * @warning Сборка с -DTEST_ENV открывает доступ к функциям сброса для тестов.
 */
#include "s21_bgame.h"

#include <stddef.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Максимальное количество поддерживаемых игр в реестре
 *
 * Эта константа определяет верхний лимит на количество игр,
 * которые могут быть зарегистрированы в библиотеке одновременно
 * через функцию bg_register_game().
 *
 * @details
 * - Используется для фиксации размера внутреннего массива реестра игр.
 * - Ограничение введено для упрощения управления памятью без использования
 * динамического выделения.
 * - Типичные значения: Tetris, Snake и другие простые игры.
 *
 * При достижении лимита новые игры игнорируются.
 *
 * @note Значение 8 выбрано как разумный компромисс между гибкостью и
 * использованием памяти. В случае необходимости значение можно изменить, но
 * следует учитывать, что это влияет на размер статического массива в реестре.
 *
 * Пример использования:
 * @code
 * static GameRegistryEntry g_registry[BG_MAX_GAMES];
 * @endcode
 * @internal
 */
#define BG_MAX_GAMES 8

/* ======================== Реестр интерфейсов игр ====================== */

/**
 * @brief Внутренняя запись в реестре зарегистрированных игр
 *
 * Структура представляет собой элемент реестра, хранящий интерфейс игры
 * и флаг её регистрации. Используется для отслеживания, какие игры
 * были зарегистрированы в библиотеке, и предотвращения дублирования.
 *
 * @details
 * - Поле `iface` содержит полный интерфейс игры (функции создания, обновления и
 * т.д.).
 * - Поле `registered` указывает, занята ли данная ячейка реестра.
 *
 * Реестр на основе этой структуры позволяет:
 * - Регистрировать игры по их уникальному ID.
 * - Получать доступ к интерфейсу игры по ID.
 * - Избегать повторной регистрации одной и той же игры.
 *
 * Используется только внутри модуля `s21_bgame.c` и не экспортируется во
 * внешние интерфейсы.
 *
 * @note Максимальное количество зарегистрированных игр ограничено константой
 * BG_MAX_GAMES.
 *
 * Пример использования:
 * ```c
 * static GameRegistryEntry g_registry[BG_MAX_GAMES];
 *
 * // При регистрации новой игры:
 * for (int i = 0; i < BG_MAX_GAMES; ++i) {
 *     if (!g_registry[i].registered) {
 *         g_registry[i].iface = iface;
 *         g_registry[i].registered = true;
 *         break;
 *     }
 * }
 * ```
 * @internal
 */
typedef struct {
  GameInterface_t iface;  ///< Интерфейс зарегистрированной игры
  bool registered;  ///< Флаг: true, если ячейка реестра занята
} GameRegistryEntry;

/**
 * @brief Глобальный реестр зарегистрированных игр
 *
 * Массив фиксированного размера, хранящий интерфейсы всех зарегистрированных
 * игр. Каждая запись содержит сам интерфейс игры и флаг регистрации.
 *
 * @details
 * - Размер массива определяется константой BG_MAX_GAMES.
 * - Инициализируется один раз при первом вызове bg_init_registry() через
 * memset.
 * - Управление производится по флагу registered в каждой записи.
 *
 * @note Является внутренней статической переменной модуля s21_bgame.c.
 *       Не доступен за пределами этого файла.
 *
 * @see bg_register_game(), bg_get_game(), g_registry_initialized
 * @internal
 */
static GameRegistryEntry g_registry[BG_MAX_GAMES];

/**
 * @brief Флаг инициализации реестра игр
 *
 * Указывает, была ли выполнена инициализация массива g_registry.
 * Используется для обеспечения однократной инициализации при первом
 * использовании (lazy initialization).
 *
 * @details
 * - Значение false при старте.
 * - Устанавливается в true при первом вызове bg_init_registry().
 * - Повторные вызовы bg_init_registry() не производят инициализацию.
 *
 * @note Обеспечивает безопасность инициализации при множественных вызовах API.
 *
 * @see bg_init_registry(), g_registry
 * @internal
 */
static bool g_registry_initialized = false;

/**
 * @brief Инициализирует реестр игр (ленивая инициализация)
 *
 * Выполняет однократную инициализацию глобального массива g_registry,
 * обнуляя все записи. Функция использует механизм lazy initialization —
 * реестр инициализируется при первом обращении, а все последующие вызовы
 * игнорируются благодаря флагу g_registry_initialized.
 *
 * @details
 * - Проверяет, была ли уже выполнена инициализация.
 * - Если нет — обнуляет массив реестра через memset.
 * - Устанавливает флаг инициализации в true.
 *
 * @note Используется внутри bg_register_game() и bg_get_game() для обеспечения
 *       потоконебезопасной (но корректной при однопоточном доступе)
 * инициализации.
 *
 * @warning Не является потокобезопасной. При многопоточном доступе возможны
 * гонки. Предполагается использование в однопоточном контексте при старте
 * приложения.
 *
 * @see bg_register_game(), bg_get_game(), g_registry, g_registry_initialized
 * @internal
 */
static void bg_init_registry(void) {
  if (g_registry_initialized) return;
  memset(g_registry, 0, sizeof(g_registry));
  g_registry_initialized = true;
}

void bg_register_game(GameInterface_t iface) {
  bg_init_registry();

  // Проверяем, не зарегистрирована ли уже игра с таким ID
  for (int i = 0; i < BG_MAX_GAMES; ++i) {
    if (g_registry[i].registered && g_registry[i].iface.id == iface.id) {
      // Игра с таким ID уже зарегистрирована — игнорируем повторную регистрацию
      return;
    }
  }

  // Ищем пустой слот в реестре
  for (int i = 0; i < BG_MAX_GAMES; ++i) {
    if (!g_registry[i].registered) {
      g_registry[i].iface = iface;
      g_registry[i].registered = true;
      return;
    }
  }

  // Реестр переполнен (молча игнорируем)
}

const GameInterface_t *bg_get_game(GameId_t id) {
  bg_init_registry();

  for (int i = 0; i < BG_MAX_GAMES; ++i) {
    if (g_registry[i].registered && g_registry[i].iface.id == id) {
      return &g_registry[i].iface;
    }
  }
  return NULL;
}

/**
 * @brief Контекст текущей активной игры
 *
 * Структура хранит ссылку на интерфейс и экземпляр игры, которая в данный
 * момент является активной в системе. Используется для управления текущей игрой
 * и обеспечения доступа к ней из функций высокого уровня, таких как
 * userInput() и updateCurrentState().
 *
 * @details
 * - Поле iface — указатель на зарегистрированный интерфейс игры
 * (GameInterface_t)
 * - Поле instance — указатель на конкретный экземпляр игры, созданный через
 * create()
 *
 * @note Является внутренней статической переменной модуля. Не экспортируется за
 * пределы файла. Управление контекстом осуществляется через bg_switch_game().
 *
 * @warning Не является потокобезопасным. Доступ к g_current должен быть
 *          синхронизирован при использовании в многопоточной среде.
 *
 * @see bg_switch_game(), userInput(), updateCurrentState()
 * @internal
 */
typedef struct {
  const GameInterface_t *iface;  ///< Указатель на интерфейс текущей игры
  void *instance;  ///< Указатель на экземпляр текущей игры
} GameContext;

/**
 * @brief Глобальный контекст активной игры
 *
 * Переменная хранит текущую игру и используется как центральная точка доступа
 * для всех операций с активной игрой: ввод, обновление, получение состояния.
 *
 * Инициализируется как {NULL, NULL}, что означает отсутствие активной игры.
 * При вызове bg_switch_game() обновляется на новую игру.
 *
 * @see GameContext, bg_switch_game()
 * @internal
 */
static GameContext g_current = {NULL, NULL};

bool bg_switch_game(GameId_t id) {
  const GameInterface_t *iface = bg_get_game(id);
  if (!iface || iface->id == GAME_UNDEFINED) {
    return false;  // Игра не зарегистрирована
  }

  // Защита от NULL-указателей в интерфейсе
  if (!iface->create || !iface->destroy || !iface->get_info || !iface->input ||
      !iface->update) {
    return false;  // Некорректный интерфейс игры
  }

  // Проверяем, не пытаемся ли мы переключиться на ту же самую игру
  if (g_current.instance && g_current.iface && g_current.iface->id == id) {
    return true;
}

  // Уничтожаем старый экземпляр, если он был
  if (g_current.instance && g_current.iface) {
    g_current.iface->destroy(g_current.instance);
    g_current.instance = NULL;
  }

  // Создаём новый экземпляр
  void *new_instance = iface->create();
  if (!new_instance) {
    g_current.iface = NULL;
    return false;  // Не удалось создать экземпляр
  }

  g_current.instance = new_instance;
  g_current.iface = iface;

  return true;
}

const GameInterface_t *bg_get_current_game(void) { return g_current.iface; }

void *bg_get_current_instance(void) { return g_current.instance; }

/* ============= Обёртка v1-интерфейса для текущей игры ================= */

void userInput(UserAction_t action, bool hold) {
  if (!g_current.iface || !g_current.instance) {
    return;
  }
  g_current.iface->input(g_current.instance, action, hold);
}

GameInfo_t updateCurrentState(void) {
  GameInfo_t info = {0};

  if (!g_current.iface || !g_current.instance) {
    return info;
  }

  g_current.iface->update(g_current.instance);
  const GameInfo_t *cur = g_current.iface->get_info(g_current.instance);
  if (cur) {
    info = *cur;
  }

  return info;
}

#ifdef TEST_ENV
/**
 * @brief Сбрасывает текущий контекст игры (для тестов)
 *
 * Устанавливает g_current.iface и g_current.instance в NULL.
 * Должно использоваться только в unit-тестах.
 *
 * @warning Используется только при сборке с -DTEST_ENV.
 * @note Функция безопасна при вызове, даже если текущая игра уже сброшена.
 */
void bg_reset_current_for_testing(void) {
  g_current.instance = NULL;
  g_current.iface = NULL;
}

void bg_reset_registry_for_testing(void) {
  memset(g_registry, 0, sizeof(g_registry));
  g_registry_initialized = false;
}
#endif