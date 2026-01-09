/**
 * @file s21_snake.cpp
 * @brief C API обёртка для C++ реализации s21::SnakeGame
 *
 * Реализует C-совместимый интерфейс для класса s21::SnakeGame, обеспечивая
 * мост между C++-логикой и C-кодом (например, игровым движком s21_bgame).
 * Все функции объявлены с linkage `extern "C"` и помечены как `noexcept`
 * для безопасного взаимодействия с C.
 *
 * @note Исключения C++ не пересекают границу C API — любое исключение
 *       будет приводить к std::terminate(). Реальная игровая логика
 *       изолирована в s21_snake_internals.cpp.
 *
 * @note Функции являются тонкими обёртками: минимально обрабатывают
 *       входные данные и делегируют логику классу SnakeGame.
 *
 * @note Потокобезопасность не гарантируется — все вызовы для одного
 *       экземпляра игры должны выполняться из одного потока.
 *
 * @see s21_snake.h        — публичный C API
 * @see s21_snake_internals.hpp — объявление класса SnakeGame
 * @see s21_snake_internals.cpp — реализация игровой логики
 * @see s21_bgame.h         — универсальный интерфейс игрового движка
 */

#include "s21_snake.h"

#include "s21_snake_internals.hpp"

extern "C" {

void* snake_create(void) { return s21::SnakeGame::create(); }

void snake_destroy(void* game) { s21::SnakeGame::destroy(game); }

void snake_handle_input(void* game, UserAction_t action, bool hold) {
  s21::SnakeGame::handle_input(game, action, hold);
}

void snake_update(void* game) { s21::SnakeGame::update(game); }

const GameInfo_t* snake_get_info(const void* game) {
  return s21::SnakeGame::get_info(game);  // без приведения!
}

GameInterface_t snake_get_interface(GameId_t id) {
  GameInterface_t iface = {GAME_UNDEFINED, nullptr, nullptr, nullptr, nullptr, nullptr,
  };

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