#include <gtest/gtest.h>

extern "C" {
#include "s21_snake.h"
}

#include "s21_snake_internals.hpp"

constexpr int kFieldHeight = 20;
constexpr int kFieldWidth = 10;
constexpr int SNAKE_HEAD = 2;
constexpr int SNAKE_BODY = 1;
constexpr int APPLE = 3;

// Базовые настройки для большинства тестов
class SnakeTest : public ::testing::Test {
 protected:
  void* game = nullptr;

  void SetUp() override {
    game = snake_create();
    ASSERT_NE(game, nullptr);
  }

  void TearDown() override {
    snake_destroy(game);
    game = nullptr;
  }

  const GameInfo_t* Info() {
    const GameInfo_t* info = snake_get_info(game);
    EXPECT_NE(info, nullptr);
    return info;
  }

  void Tick(int n = 1) {
    for (int i = 0; i < n; ++i) {
      snake_update(game);
    }
  }
};

std::pair<int, int> FindHead(const GameInfo_t* info) {
  for (int y = 0; y < 20; ++y) {
    for (int x = 0; x < 10; ++x) {
      if (info->field[y][x] == SNAKE_HEAD) {
        return {y, x};
      }
    }
  }
  return {-1, -1};  // Голова не найдена
}

bool FieldEquals(const GameInfo_t* a, const GameInfo_t* b) {
  for (int y = 0; y < 20; ++y)
    for (int x = 0; x < 10; ++x)
      if (a->field[y][x] != b->field[y][x])
        return false;
  return true;
}

/* ===== API и интерфейс ===== */

TEST(SnakeApiTest, CreateDestroyNullSafe) {
  // NULL в destroy не должен падать
  snake_destroy(nullptr);

  // Проверка, что get_info(nullptr) возвращает nullptr
  EXPECT_EQ(snake_get_info(nullptr), nullptr);

  void* game = snake_create();
  ASSERT_NE(game, nullptr);

  const GameInfo_t* info = snake_get_info(game);
  ASSERT_NE(info, nullptr);

  // Проверка указателей
  EXPECT_NE(info->field, nullptr) << "Field should be allocated";
  EXPECT_NE(info->next, nullptr) << "Next field should be allocated";

  // Проверка начальных значений
  EXPECT_EQ(info->score, 0) << "Initial score must be 0";
  EXPECT_EQ(info->high_score, 0) << "High score should start at 0 (or load properly)";
  EXPECT_EQ(info->level, 1) << "Initial level must be 1";
  EXPECT_GE(info->speed, 1) << "Speed must be at least 1";
  EXPECT_LE(info->speed, 10) << "Speed must be reasonable (<= 10)";
  EXPECT_EQ(info->pause, 0) << "Game should start unpaused";

  // Опционально: проверка, что поле не мусорное (хотя бы первые ячейки)
  // Это важно, если snake_create() не инициализирует field
  bool field_zeroed = true;
  for (int i = 0; i < kFieldHeight && field_zeroed; ++i) {
    for (int j = 0; j < kFieldWidth; ++j) {
      if (info->field[i][j] != 0 && info->field[i][j] != 1 && info->field[i][j] != 2 && info->field[i][j] != 3) {
        field_zeroed = false;
        break;
      }
    }
  }
  EXPECT_TRUE(field_zeroed) << "Field contains invalid cell values after create";

  snake_destroy(game);
  snake_destroy(nullptr);  // Повторный вызов — должен быть безопасен
}

TEST(SnakeApiTest, GetInterfaceSnake) {
  GameInterface_t iface = snake_get_interface(GAME_SNAKE);
  EXPECT_EQ(iface.id, GAME_SNAKE);
  EXPECT_NE(iface.create, nullptr);
  EXPECT_NE(iface.destroy, nullptr);
  EXPECT_NE(iface.input, nullptr);
  EXPECT_NE(iface.update, nullptr);
  EXPECT_NE(iface.get_info, nullptr);
}

TEST(SnakeApiTest, GetInterfaceInvalid) {
  GameInterface_t iface = snake_get_interface(GAME_TETRIS);
  EXPECT_EQ(iface.id, GAME_UNDEFINED);
  EXPECT_EQ(iface.create, nullptr);
  EXPECT_EQ(iface.get_info, nullptr);
}

/* ===== Начальное состояние и INIT ===== */

TEST_F(SnakeTest, FieldStableAndEmptyInInit) {
  const GameInfo_t* info1 = Info();
  EXPECT_EQ(info1->pause, 0);

  // Делаем снимок поля
  int before[20][10];
  for (int y = 0; y < 20; ++y)
    for (int x = 0; x < 10; ++x)
      before[y][x] = info1->field[y][x];

  Tick(5);
  const GameInfo_t* info2 = Info();
  bool changed = false;
  for (int y = 0; y < 20; ++y)
    for (int x = 0; x < 10; ++x)
      if (before[y][x] != info2->field[y][x]) changed = true;

  EXPECT_FALSE(changed);
}

TEST_F(SnakeTest, InitialStateFieldAndSnake) {
  snake_handle_input(game, Start, false);
  Tick(1);
  
  const GameInfo_t* info = Info();
  int snake_len = 0, apples = 0, head_count = 0;
  // Проверяем, что поле корректно инициализировано:
  // только 0 (пусто), 1 (тело змейки), 2 (голова), 3 (яблоко))
  for (int y = 0; y < kFieldHeight; ++y) {
    for (int x = 0; x < kFieldWidth; ++x) {
        int cell = info->field[y][x];
        switch (cell) {
            case 0: break;
            case SNAKE_BODY: case SNAKE_HEAD:
                ++snake_len;
                if (cell == SNAKE_HEAD) ++head_count;
                break;
            case APPLE: ++apples; break;
            default:
                FAIL() << "Invalid cell value at [" << y << "][" << x << "]: " << cell;
                return;
        }
    }
  }
  EXPECT_GE(snake_len, 3);   // начальная длина
  EXPECT_EQ(head_count, 1); // голова одна штука
  EXPECT_EQ(apples, 1);   // одно яблоко
}

// /* ===== Движение и изменение направления ===== */

TEST_F(SnakeTest, MoveRightByDefault) {
  // Начинаем игру
  snake_handle_input(game, Start, false);

  // Делаем один тик — чтобы увидеть первое движение
  Tick(1);
  const GameInfo_t* info1 = Info();
  auto [y1, x1] = FindHead(info1);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна после Start";

  // Ещё один тик — проверяем движение
  Tick(1);
  const GameInfo_t* info2 = Info();
  auto [y2, x2] = FindHead(info2);
  ASSERT_NE(x2, -1) << "Голова змейки не должна исчезать";

  // Проверяем: осталась на той же строке, сместилась вправо на 1
  EXPECT_EQ(y1, y2) << "Змейка должна двигаться горизонтально";
  EXPECT_EQ(x2, x1 + 1) << "Змейка должна двигаться вправо по умолчанию";
}

TEST_F(SnakeTest, ChangeDirectionUpValid) {
  snake_handle_input(game, Start, false);
  Tick(1);
  const GameInfo_t* info1 = Info();
  auto [y1, x1] = FindHead(info1);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна после Start";

  snake_handle_input(game, Up, false);
  Tick(1);
  const GameInfo_t* info2 = Info();
  auto [y2, x2] = FindHead(info2);
  ASSERT_NE(x2, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y1 - 1 , y2) << "Змейка должна двигаться вверх";
  EXPECT_EQ(x2, x1) << "Змейка не должна двигаться по горизонтали";
}

TEST_F(SnakeTest, ChangeDirectionDownValid) {
  snake_handle_input(game, Start, false);
  Tick(1);
  const GameInfo_t* info1 = Info();
  auto [y1, x1] = FindHead(info1);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна после Start";

  snake_handle_input(game, Down, false);
  Tick(1);
  const GameInfo_t* info2 = Info();
  auto [y2, x2] = FindHead(info2);
  ASSERT_NE(x2, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y1 + 1 , y2) << "Змейка должна двигаться вниз";
  EXPECT_EQ(x2, x1) << "Змейка не должна двигаться по горизонтали";
}

TEST_F(SnakeTest, ChangeDirectionDownLeftValid) {
  snake_handle_input(game, Start, false);
  Tick(1);
  const GameInfo_t* info1 = Info();
  auto [y1, x1] = FindHead(info1);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна после Start";

  snake_handle_input(game, Down, false);
  Tick(1);
  const GameInfo_t* info2 = Info();
  auto [y2, x2] = FindHead(info2);
  ASSERT_NE(x2, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y1 + 1 , y2) << "Змейка должна двигаться вниз";
  EXPECT_EQ(x2, x1) << "Змейка не должна двигаться по горизонтали";
  
  snake_handle_input(game, Left, false);
  Tick(1);
  const GameInfo_t* info3 = Info();
  auto [y3, x3] = FindHead(info3);
  ASSERT_NE(x3, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y3 , y2) << "Змейка не должна двигаться по вертикали";
  EXPECT_EQ(x3, x1 - 1) << "Змейка должна двигаться влево";
}

TEST_F(SnakeTest, ChangeDirectionUpRightValid) {
  snake_handle_input(game, Start, false);
  Tick(1);
  const GameInfo_t* info1 = Info();
  auto [y1, x1] = FindHead(info1);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна после Start";

  snake_handle_input(game, Up, false);
  Tick(1);
  const GameInfo_t* info2 = Info();
  auto [y2, x2] = FindHead(info2);
  ASSERT_NE(x2, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y1 - 1 , y2) << "Змейка должна двигаться вверх";
  EXPECT_EQ(x2, x1) << "Змейка не должна двигаться по горизонтали";

  snake_handle_input(game, Right, false);
  Tick(1);
  const GameInfo_t* info3 = Info();
  auto [y3, x3] = FindHead(info3);
  ASSERT_NE(x3, -1) << "Голова змейки не должна исчезать";

  EXPECT_EQ(y3 , y2) << "Змейка не должна двигаться по вертикали";
  EXPECT_EQ(x3, x1 + 1) << "Змейка должна двигаться влево";
}

TEST_F(SnakeTest, PreventReverseDirection) {
  // Инициализация
  snake_handle_input(game, Start, false);
  Tick(1);

  auto [y1, x1] = FindHead(Info());
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна";

  // Попытка разворота на 180°: вправо → влево
  snake_handle_input(game, Left, false);
  Tick(1);

  auto [y2, x2] = FindHead(Info());
  ASSERT_NE(x2, -1) << "Голова не должна исчезнуть";

  // Проверяем, что направление не изменилось: движение продолжается вправо
  EXPECT_EQ(x2, x1 + 1) << "Змейка продолжает движение вправо";
  EXPECT_EQ(y2, y1)    << "Координата Y не должна измениться";
}

// /* ===== Пауза и возобновление ===== */

TEST_F(SnakeTest, PauseAndResume) {
  snake_handle_input(game, Start, false);
  Tick(1);
  const GameInfo_t* before = Info();
  EXPECT_EQ(before->pause, 0) << "Игра не должна быть на паузе";
  auto [y1, x1] = FindHead(before);
  ASSERT_NE(x1, -1) << "Голова змейки должна быть видна";

  // Пауза
  snake_handle_input(game, Pause, false);
  const GameInfo_t* paused = Info();
  EXPECT_EQ(paused->pause, 1) << "Игра должна встать на паузу";
  auto [y2, x2] = FindHead(paused);
  ASSERT_NE(x2, -1) << "Голова змейки должна быть видна";

  // В паузе поле не меняется
  Tick(3);
  const GameInfo_t* paused_after = Info();
  auto [y3, x3] = FindHead(paused_after);
  ASSERT_NE(x2, -1) << "Голова змейки должна быть видна";
  EXPECT_EQ(x3, x2) << "Координата X не должна двигаться";
  EXPECT_EQ(y3, y2)    << "Координата Y не должна измениться";

  // Снятие с паузы
  snake_handle_input(game, Pause, false);
  const GameInfo_t* resumed = Info();
  EXPECT_EQ(resumed->pause, 0) << "Игра должна быть снята с паузы";
  auto [y4, x4] = FindHead(resumed);
  ASSERT_NE(x4, -1) << "Голова змейки должна быть видна";

  Tick(1);
  const GameInfo_t* moved = Info();
  auto [y5, x5] = FindHead(moved);
  ASSERT_NE(x5, -1) << "Голова змейки должна быть видна";
  EXPECT_EQ(x5, x4 + 1) << "Змейка должна сместиться вправо";
  EXPECT_EQ(y5, y4)    << "Координата Y не должна измениться";
}

// /* ===== Столкновения: стены и самосъедание ===== */

TEST_F(SnakeTest, CollisionWithRightWall) {
  // 1. Запускаем игру
  snake_handle_input(game, Start, false);
  Tick(1);

  const GameInfo_t* info = Info();
  auto [head_y, head_x] = FindHead(info);
  ASSERT_NE(head_x, -1) << "Голова змейки не найдена";

  // 2. Двигаем змейку вправо до правой границы
  // Предполагаем, что направление — вправо (по умолчанию)
  const int steps_needed = 10 - 1 - head_x;
  ASSERT_GT(steps_needed, 0) << "Змейка уже у правой стены";

  for (int i = 0; i < steps_needed; ++i) {
    Tick(1);
  }

  // После этого голова должна быть на x = 9 (для ширины 10)
  info = Info();
  auto [y_after, x_after] = FindHead(info);
  EXPECT_EQ(x_after, 10 - 1) << "Змейка должна достичь правой стены";

  // 3. Делаем шаг вправо — за границу (столкновение)
  Tick(1);

  // 4. Запоминаем состояние ПОСЛЕ столкновения
  const GameInfo_t* post_collision = Info();

  // 5. Делаем ещё несколько тиков — если игра "умерла", поле не должно меняться
  const int freeze_ticks = 5;
  for (int i = 0; i < freeze_ticks; ++i) {
    Tick(1);
  }

  const GameInfo_t* frozen = Info();

  // 6. Сравниваем: поле не изменилось?
  bool field_changed = false;
  for (int y = 0; y < 20; ++y) {
    for (int x = 0; x < 10; ++x) {
      if (post_collision->field[y][x] != frozen->field[y][x]) {
        field_changed = true;
        break;
      }
    }
    if (field_changed) break;
  }

  EXPECT_FALSE(field_changed) << "Поле должно замереть после столкновения (GAME_OVER)";
}

TEST_F(SnakeTest, SelfCollision_WithControlledFood) {
  // 1. Запускаем игру
  snake_handle_input(game, Start, false);
  Tick(1);

  const GameInfo_t* info = Info();
  auto [head_y, head_x] = FindHead(info);
  ASSERT_NE(head_x, -1) << "Голова змейки не найдена";

  // 2. Удлиняем змейку, подкладывая еду
  int grow_steps = 4;  // до длины 7
  for (int i = 0; i < grow_steps; ++i) {
    // Ищем свободную клетку впереди (вправо)
    int food_x = head_x + 2 + i;  // чуть дальше
    int food_y = head_y;

    // Защита от выхода за границу
    if (food_x >= 10) food_x = 10 - 2;

    // Устанавливаем еду
#ifdef SNAKE_TEST_ACCESS
  // Только если мы можем видеть SnakeGame
  s21::SnakeGame* real_game = reinterpret_cast<s21::SnakeGame*>(game);
  real_game->set_food_for_testing(food_x, food_y);
#else
  GTEST_SKIP() << "Тест требует SNAKE_TEST_ACCESS";
#endif

    // Двигаемся к еде
    while (FindHead(Info()).second < food_x) {
      snake_handle_input(game, Right, false);
      Tick(1);
    }
    // Делаем шаг — съедаем
    Tick(1);

    // Обновляем позицию головы
    std::tie(head_y, head_x) = FindHead(Info());
  }

  // 3. Теперь змейка длинная — делаем U-поворот
  snake_handle_input(game, Up, false);
  Tick(2);
  snake_handle_input(game, Left, false);
  Tick(2);
  snake_handle_input(game, Down, false);
  Tick(3);  // теперь голова должна въехать в тело

  // 4. Проверяем: поле замерло
  const GameInfo_t* pre_freeze = Info();
  Tick(5);
  const GameInfo_t* post_freeze = Info();

  EXPECT_TRUE(FieldEquals(pre_freeze, post_freeze))
      << "Поле должно замереть после столкновения с телом";
}

// /* ===== Съедение яблока, рост и счёт ===== */

// TEST_F(SnakeTest, EatAppleIncreaseScoreAndLength) {
//   snake_handle_input(game, Start, false);
//   const GameInfo_t* info = Info();

//   int apple_y = -1, apple_x = -1;
//   for (int y = 0; y < 20 && apple_x == -1; ++y) {
//     for (int x = 0; x < 10; ++x) {
//       if (info->field[y][x] == 2) {
//         apple_y = y;
//         apple_x = x;
//         break;
//       }
//     }
//   }
//   ASSERT_GE(apple_x, 0);

//   int initial_score = info->score;
//   int initial_snake_cells = 0;
//   for (int y = 0; y < 20; ++y)
//     for (int x = 0; x < 10; ++x)
//       if (info->field[y][x] == 1) ++initial_snake_cells;

//   int steps = 100;
//   while (steps-- > 0) {
//     const GameInfo_t* cur = Info();

//     int head_y = -1, head_x = -1;
//     for (int y = 0; y < 20 && head_x == -1; ++y) {
//       for (int x = 0; x < 10; ++x) {
//         if (cur->field[y][x] == 1) {
//           head_y = y;
//           head_x = x;
//           break;
//         }
//       }
//     }

//     if (head_x == apple_x && head_y == apple_y) break;

//     if (head_y > apple_y) snake_handle_input(game, Up, false);
//     else if (head_y < apple_y) snake_handle_input(game, Down, false);
//     else if (head_x < apple_x) snake_handle_input(game, Right, false);
//     else if (head_x > apple_x) snake_handle_input(game, Left, false);

//     Tick(1);
//   }

//   const GameInfo_t* after = Info();
//   EXPECT_GT(after->score, initial_score);

//   int snake_cells = 0;
//   for (int y = 0; y < 20; ++y)
//     for (int x = 0; x < 10; ++x)
//       if (after->field[y][x] == 1) ++snake_cells;

//   EXPECT_GT(snake_cells, initial_snake_cells);
// }

/* ===== Terminate и повторный старт ===== */

TEST_F(SnakeTest, TerminateAndRestart) {
  // 1. Запускаем игру
  snake_handle_input(game, Start, false);
  Tick(1);

  const GameInfo_t* running = Info();
  auto [head_y, head_x] = FindHead(running);
  ASSERT_NE(head_x, -1) << "Змейка должна быть на поле после Start";

  // 2. Отправляем Terminate → GAME_OVER
  snake_handle_input(game, Terminate, false);
  Tick(1);

  const GameInfo_t* terminated = Info();
  EXPECT_EQ(terminated->pause, 0) << "Пауза не должна быть активна";

  // 3. Проверяем: поле не меняется (заморозка)
  const GameInfo_t* pre_freeze = Info();
  Tick(3);
  const GameInfo_t* post_freeze = Info();

  EXPECT_TRUE(FieldEquals(pre_freeze, post_freeze))
      << "Поле должно быть заморожено после Terminate";

  // 4. Отправляем Start → переход в INIT
  snake_handle_input(game, Start, false);
  Tick(1);
  // 5. Отправляем Start -> пепреход в Move
  snake_handle_input(game, Start, false);
  Tick(1);

  const GameInfo_t* restarted = Info();
  auto [new_y, new_x] = FindHead(restarted);

  // 5. Проверяем: новая змейка появилась
  EXPECT_NE(new_x, -1) << "Новая змейка должна появиться после Start";
  EXPECT_TRUE(
      restarted->field[new_y][new_x] == 1
  ) << "Голова змейки должна быть на новой позиции";

  // 6. Поле изменилось по сравнению с заморожённым состоянием
  EXPECT_FALSE(FieldEquals(post_freeze, restarted))
      << "Поле должно измениться после перезапуска";

  // 7. Проверяем: счёт сброшен?
  EXPECT_EQ(restarted->score, 0)
        << "Счёт должен сбрасываться при новом старте";

  // 8. Двигаемся — поле должно меняться
  const GameInfo_t* before_move = Info();
  Tick(1);
  const GameInfo_t* after_move = Info();

  EXPECT_FALSE(FieldEquals(before_move, after_move))
      << "Змейка должна двигаться после перезапуска";
}

// /* ===== NULL‑безопасность ===== */

// TEST(SnakeNullTest, NullSafeApi) {
//   snake_handle_input(nullptr, Start, false);
//   snake_update(nullptr);
//   const GameInfo_t* info = snake_get_info(nullptr);
//   EXPECT_EQ(info, nullptr);
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}