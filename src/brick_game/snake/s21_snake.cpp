/**
 * @file s21_snake.cpp
 * @brief C API обёртка для C++ реализации s21::SnakeGame
 *
 * Мостует C++ класс s21::SnakeGame с C-интерфейсом, используемым в s21_bgame.
 * Все функции объявлены как extern "C" для совместимости с C-линкером.
 *
 * Все функции помечены как noexcept, чтобы предотвратить пересечение границы C/C++
 * исключений C++, что приведёт к std::terminate().
 *
 * @note Реальная логика игры находится в s21_snake_internals.cpp.
 * @see s21_snake.h, s21_snake_internals.hpp, s21_bgame.h
 */

#include "s21_snake.h"
#include "s21_snake_internals.hpp"

#include <cassert>

extern "C" {

void* snake_create(void) noexcept { return s21::SnakeGame::create(); }

void snake_destroy(void* game) noexcept { s21::SnakeGame::destroy(game); }

void snake_handle_input(void* game, UserAction_t action, bool hold) noexcept {
  s21::SnakeGame::handle_input(game, action, hold);
}

void snake_update(void* game) noexcept { s21::SnakeGame::update(game); }

const GameInfo_t* snake_get_info(const void* game) noexcept {
    return s21::SnakeGame::get_info(game);  // без приведения!
}

GameInterface_t snake_get_interface(GameId_t id) noexcept{
  assert((id == GAME_SNAKE) && "snake_get_interface: ожидается только GAME_SNAKE");

  GameInterface_t iface = {0};
  if (id == GAME_SNAKE) {
    iface.id = id;
    iface.create = snake_create;
    iface.destroy = snake_destroy;
    iface.input = snake_handle_input;
    iface.update = snake_update;
    iface.get_info = snake_get_info;
  }

  return iface;
}

}  // extern "C"