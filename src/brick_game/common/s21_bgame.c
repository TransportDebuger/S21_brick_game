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
#include <stdlib.h>
#include <dlfcn.h> // Для dlopen, dlsym, dlclose

/* ======================== Реестр интерфейсов игр ====================== */


/**
 * @brief Внутренняя запись в реестре зарегистрированных игр
 *
 * Структура представляет собой элемент реестра, хранящий интерфейс игры,
 * дескриптор динамической библиотеки (для плагинов), путь к файлу и флаг её регистрации.
 * Используется для отслеживания, какие игры были зарегистрированы в библиотеке.
 *
 * @details
 * - Поле `iface` содержит полный интерфейс игры (функции создания, обновления и т.д.).
 * - Поле `library_handle` содержит дескриптор, возвращённый dlopen(). NULL для встроенных игр.
 * - Поле `library_path` содержит копию пути к .so/.dll файлу. NULL для встроенных игр.
 * - Поле `registered` указывает, занята ли данная ячейка реестра.
 *
 * Реестр на основе этой структуры позволяет:
 * - Регистрировать игры по их уникальному ID.
 * - Различать встроенные и динамически загруженные игры.
 * - Корректно выгружать динамические библиотеки.
 *
 * @note Используется только внутри модуля `s21_bgame.c` и не экспортируется во внешние интерфейсы.
 *
 * @see s21_bgame.c::g_registry, s21_bgame.c::g_registry_capacity, s21_bgame.c::g_registry_size
 * @internal
 */
typedef struct {
  GameInterface_t iface;  ///< Интерфейс зарегистрированной игры
  void* library_handle;   ///< Дескриптор динамической библиотеки (dlopen/LoadLibrary), NULL для встроенных игр.
  char* library_path;     ///< Путь к .so/.dll файлу, NULL для встроенных игр.
  bool registered;        ///< Флаг: true, если ячейка реестра занята.
} GameRegistryEntry;


/**
 * @brief Глобальный реестр зарегистрированных игр (динамический массив)
 *
 * Указатель на динамически выделяемый массив записей. Размер массива может изменяться.
 *
 * @note Заменяет статический массив `g_registry[BG_MAX_GAMES]`. Память выделяется через malloc/realloc.
 * @see bg_register_game(), registry_ensure_capacity()
 * @internal
 */
static GameRegistryEntry* g_registry = NULL;

/**
 * @brief Текущая ёмкость (capacity) динамического массива реестра.
 *
 * Количество элементов, которое может вместить `g_registry` без перераспределения памяти.
 * @note Инициализируется 0.
 * @see g_registry, g_registry_size
 * @internal
 */
static int g_registry_capacity = 0;

/**
 * @brief Текущее количество активных записей в реестре.
 *
 * Количество игр, для которых `registered` == true.
 * @note Инициализируется 0.
 * @see g_registry, g_registry_capacity
 * @internal
 */
static int g_registry_size = 0;


/**
 * @brief Инициализирует динамический реестр игр (ленивая инициализация)
 *
 * Выполняет однократную инициализацию `g_registry` как NULL и сбрасывает счётчики.
 * Функция использует механизм lazy initialization — реестр инициализируется при первом обращении.
 *
 * @details
 * - Проверяет, была ли уже выполнена инициализация (`g_registry_capacity` == 0).
 * - Если нет — устанавливает `g_registry` в NULL, `g_registry_size` в 0.
 *   Реальное выделение памяти произойдет в `registry_ensure_capacity` при первой регистрации.
 *
 * @note Используется внутри bg_register_game() и bg_get_game() для обеспечения инициализации.
 * @warning Не является потокобезопасной.
 * @see bg_register_game(), bg_get_game(), registry_ensure_capacity()
 * @internal
 */
static void registry_init(void) {
  if (g_registry_capacity != 0) return; // Уже инициализирован
  g_registry = NULL;
  g_registry_size = 0;
  g_registry_capacity = 0; // Обозначает, что инициализация прошла
}

/**
 * @brief Обеспечивает доста��очную ёмкость динамического массива реестра.
 *
 * Увеличивает размер `g_registry` до `min_capacity`, если текущая ёмкость меньше требуемой.
 * Использует стратегию удвоения (или больше), чтобы минимизировать количество вызовов `realloc`.
 *
 * @param[in] min_capacity Минимально требуемая ёмкость массива.
 * @return true, если память была успешно выделена или уже была достаточной; false при ошибке выделения.
 *
 * @details
 * 1. Если текущая ёмкость >= `min_capacity`, функция возвращает true без изменений.
 * 2. Вычисляет новую ёмкость как максимальное значение из `min_capacity` и `g_registry_capacity * 2`.
 * 3. Выделяет память для нового массива через `realloc`.
 * 4. Если `realloc` успешен, копирует старые данные в новый массив, обновляет `g_registry` и `g_registry_capacity`.
 * 5. Возвращает результат.
 *
 * @note При первом вызове (g_registry == NULL) функция работает как `malloc`.
 * @warning Не является потокобезопасной. Не проверяет, что `min_capacity` положительно.
 * @see registry_init(), bg_register_game()
 * @internal
 */
static bool registry_ensure_capacity(int min_capacity) {
  if (g_registry_capacity >= min_capacity) {
    return true; // Ёмкость достаточна
  }

  int new_capacity = min_capacity;
  if (new_capacity < 4) new_capacity = 4; // Минимальный размер
  if (new_capacity < g_registry_capacity * 2) new_capacity = g_registry_capacity * 2; // Удвоение

  GameRegistryEntry* new_registry = (GameRegistryEntry*) realloc(g_registry, new_capacity * sizeof(GameRegistryEntry));
  if (!new_registry) {
    return false; // Ошибка выделения памяти
  }

  // Если realloc увеличил память, инициализируем новые элементы
  if (g_registry_capacity > 0 && new_capacity > g_registry_capacity) {
    memset(&new_registry[g_registry_capacity], 0, (new_capacity - g_registry_capacity) * sizeof(GameRegistryEntry));
  }

  g_registry = new_registry;
  g_registry_capacity = new_capacity;
  return true;
}

/**
 * @brief Ищет запись в реестре по идентификатору игры.
 *
 * Проходит по всем активным записям реестра и возвращает индекс первой,
 * у которой `registered` == true и `iface.id` совпадает с `id`.
 *
 * @param[in] id Идентификатор игры (GameId_t) для поиска.
 * @return Индекс записи в массиве `g_registry`, если найдена; -1, если игра не найдена.
 *
 * @note Функция предполагает, что `g_registry` инициализирован (registry_init вызван).
 * @warning Не является потокобезопасной.
 * @see bg_get_game(), bg_load_plugin()
 * @internal
 */
static int registry_find_by_id(GameId_t id) {
  for (int i = 0; i < g_registry_capacity; ++i) {
    if (g_registry[i].registered && g_registry[i].iface.id == id) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Добавляет новую запись в реестр игр.
 *
 * Находит первую пустую ячейку в реестре и копирует в неё переданную запись.
 * Перед добавлением гарантирует, что в массиве достаточно места.
 *
 * @param[in] entry Указатель на запись `GameRegistryEntry`, которую нужно добавить.
 * @return true, если запись была успешно добавлена; false при ошибке выделения памяти.
 *
 * @details
 * 1. Проверяет, что в реестре есть свободное место. Если нет, вызывает `registry_ensure_capacity`.
 * 2. Ищет индекс первой незарегистрированной ячейки.
 * 3. Копирует данные из `*entry` в найденную ячейку.
 * 4. Устанавливает `registered` в `true`.
 * 5. Увеличивает `g_registry_size`.
 * 6. Возвращает `true`.
 *
 * @note Функция не проверяет дубликаты по `id`. Это должна делать вызывающая функция.
 * @warning Не является потокобезопасной.
 * @see bg_register_game(), bg_load_plugin()
 * @internal
 */
static bool registry_add_entry(const GameRegistryEntry* entry) {
  // Гарантируем, что есть место хотя бы для одного элемента
  if (!registry_ensure_capacity(g_registry_size + 1)) {
    return false;
  }

  // Ищем первую пустую ячейку
  for (int i = 0; i < g_registry_capacity; ++i) {
    if (!g_registry[i].registered) {
      g_registry[i] = *entry;
      g_registry[i].registered = true;
      g_registry_size++;
      return true;
    }
  }
  // Этот код не должен выполняться, если registry_ensure_capacity отработал правильно
  return false;
}

/**
 * @brief Удаляет запись из реестра игр по индексу.
 *
 * Помечает ячейку как незарегистрированную, уменьшает счётчик размера и сдвигает
 * все последующие элементы на одну позицию влево, чтобы заполнить образовавшуюся дыру.
 * Освобождает память, выделенную для `library_path`.
 *
 * @param[in] index Индекс записи, которую нужно удалить.
 *
 * @details
 * 1. Проверяет, что индекс в допустимых пределах и запись зарегистрирована.
 * 2. Если `library_path` не NULL, освобождает его через `free`.
 * 3. Устанавливает `registered` в `false`.
 * 4. Сдвигает все элементы с индекса `index+1` до конца массива на одну позицию влево.
 * 5. Уменьшает `g_registry_size`.
 *
 * @note Вызывается только `bg_unload_all_plugins` и только для динамически загруженных плагинов.
 * @warning Не является потокобезопасной. Не проверяет валидность индекса.
 * @see bg_unload_all_plugins()
 * @internal
 */
static void registry_remove_entry(int index) {
  if (index < 0 || index >= g_registry_capacity || !g_registry[index].registered) {
    return;
  }

  // Освобождаем память, выделенную под путь к библиотеке
  if (g_registry[index].library_path) {
    free(g_registry[index].library_path);
    g_registry[index].library_path = NULL;
  }

  // Помечаем как незарегистрированное
  g_registry[index].registered = false;

  // Сдвигаем оставшиеся элементы влево, чтобы заполнить дыру
  if (index < g_registry_capacity - 1) {
    memmove(&g_registry[index], &g_registry[index + 1], (g_registry_capacity - index - 1) * sizeof(GameRegistryEntry));
  }
  // Обнуляем последний элемент
  memset(&g_registry[g_registry_capacity - 1], 0, sizeof(GameRegistryEntry));

  g_registry_size--;
}

void bg_register_game(GameInterface_t iface) {
  registry_init();

  // Проверяем, не зарегистрирована ли уже игра с таким ID
  int index = registry_find_by_id(iface.id);
  if (index != -1) {
    // Игра с таким ID уже зарегистрирована — игнорируем повторную регистрацию
    return;
  }

  // Создаём новую запись
  GameRegistryEntry entry = {0};
  entry.iface = iface;
  // Для встроенных игр поля library_handle и library_path оставляем NULL

  if (!registry_add_entry(&entry)) {
    // Реестр переполнен или ошибка выделения памяти (молча игнорируем)
  }
}

const GameInterface_t *bg_get_game(GameId_t id) {
  registry_init();

  int index = registry_find_by_id(id);
  if (index != -1) {
    return &g_registry[index].iface;
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

int bg_load_plugin(const char *path) {
  // 1. Проверка существования файла (упрощённо)
  if (!path || access(path, F_OK) != 0) {
    return -1; // Файл не найден или недоступен
  }

  // 2. Загрузка динамической библиотеки
  void* handle = dlopen(path, RTLD_LAZY);
  if (!handle) {
    return -2; // Не удалось загрузить библиотеку
  }

  // 3. Получение адреса функции get_game_interface
  GameInterface_t (*get_game_interface_func)(void);
  *(void**)(&get_game_interface_func) = dlsym(handle, "get_game_interface");
  if (!get_game_interface_func) {
    dlclose(handle);
    return -3; // Символ не найден
  }

  // 4. Вызов функции для получения интерфейса
  GameInterface_t iface = get_game_interface_func();
  if (iface.id == GAME_UNDEFINED) {
    dlclose(handle);
    return -3; // Некорректный интерфейс
  }

  // 5. Проверка, не зарегистрирована ли уже игра с таким ID
  if (bg_get_game(iface.id)) {
    dlclose(handle);
    return -4; // Дубликат
  }

  // 6. Создание записи для реестра
  GameRegistryEntry entry = {0};
  entry.iface = iface;
  entry.library_handle = handle;
  // Копируем путь к файлу
  entry.library_path = strdup(path); // Требует <string.h>
  if (!entry.library_path) {
    dlclose(handle);
    return -2; // Ошибка выделения памяти
  }

  // 7. Добавление в реестр
  if (!registry_add_entry(&entry)) {
    dlclose(handle);
    free(entry.library_path);
    return -2; // Ошибка добавления в реестр
  }

  return 0; // Успех
}

int bg_unload_all_plugins(void) {
  // Проходим в обратном порядке, чтобы при удалении индексы оставались валидными
  for (int i = g_registry_capacity - 1; i >= 0; i--) {
    if (g_registry[i].registered && g_registry[i].library_handle) {
      // Если эта игра является текущей, уничтожаем её экземпляр
      if (g_current.iface == &g_registry[i].iface && g_current.instance) {
        g_current.iface->destroy(g_current.instance);
        g_current.instance = NULL;
        g_current.iface = NULL;
      }
      // Выгружаем библиотеку
      dlclose(g_registry[i].library_handle);
      // Удаляем запись из реестра (это также освободит library_path)
      registry_remove_entry(i);
    }
  }
  return 0;
}

int bg_get_registered_count(void) {
  // registry_init(); // Вызов не нужен, т.к. registry_find_by_id вызывает registry_init
  return g_registry_size;
}

int bg_list_registered_games(GameId_t *buffer, int buffer_size) {
  if (!buffer || buffer_size <= 0) return 0;

  int count = 0;
  for (int i = 0; i < g_registry_capacity && count < buffer_size; ++i) {
    if (g_registry[i].registered) {
      buffer[count++] = g_registry[i].iface.id;
    }
  }
  return count;
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
  // Выгружаем все плагины
  bg_unload_all_plugins();
  // Освобождаем память динамического массива
  free(g_registry);
  g_registry = NULL;
  g_registry_capacity = 0;
  g_registry_size = 0;
}
#endif