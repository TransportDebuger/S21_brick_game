#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "../gui/common/view.h"   // View_t, ViewHandle_t, ElementData_t, InputEvent_t
#include "../gui/cli/cli.h"       // cli_view extern const View_t

// FPS только для плавного опроса
enum { TARGET_FPS = 30 };

// простая задержка по FPS
static void sleep_until_next_frame(int fps) {
    const long frame_ns = 1000000000L / fps;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = frame_ns };
    nanosleep(&ts, NULL);
}

// отрисовать текстовое меню в зоне "field"
static void draw_main_menu(ViewHandle_t view) {
    const char *menu_text =
        "Main menu:\n"
        "1 - Tetris\n"
        "2 - Snake\n"
        "0 - Exit\n";

    ElementData_t data = {
        .type = ELEMENT_TEXT,
        .content.text = menu_text
    };

    ViewResult_t res = cli_view.draw_element(view, "field", &data);
    if (res != VIEW_OK) {
        fprintf(stderr, "draw_element failed: %d\n", res);
        return;
    }

    res = cli_view.render(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "render failed: %d\n", res);
    }
}

// Отрисовка меню для выбранной игры
static void draw_game_menu(ViewHandle_t view, int game_id) {
    const char *game_name = (game_id == 1) ? "Tetris" : "Snake";
    char buf[128];
    snprintf(buf, sizeof(buf),
             "%s:\n"
             "1 - Start\n"
             "9 - Back\n"
             "0 - Exit\n",
             game_name);

    ElementData_t data = { .type = ELEMENT_TEXT, .content.text = buf };

    ViewResult_t res = cli_view.draw_element(view, "field", &data);
    if (res != VIEW_OK) {
        fprintf(stderr, "draw_element failed: %d\n", res);
        return;
    }

    res = cli_view.render(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "render failed: %d\n", res);
    }
}

int main(void) {
    const int field_w = 20;
    const int field_h = 10;

    ViewHandle_t view = cli_view.init(field_w, field_h, TARGET_FPS);
    if (!view) {
        fprintf(stderr, "Failed to init CLI view\n");
        return 1;
    }

    // Проверка configure_zone
    ViewResult_t res = cli_view.configure_zone(view, "field", 2, 2, 30, 10);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }

    bool running = true;
    while (running) {
        int chosen_game = -1;

        // Главное меню
        while (running && chosen_game == -1) {
            draw_main_menu(view);

            InputEvent_t ev = {0};
            res = cli_view.poll_input(view, &ev);
            if (res == VIEW_OK) {
                switch (ev.key_code) {
                    case '1': chosen_game = 1; break;
                    case '2': chosen_game = 2; break;
                    case '0': running = false;  break;
                    default: break;
                }
            }
            // Если VIEW_NO_EVENT — просто продолжать
            // Если VIEW_ERROR — это проблема, но можно игнорировать в demo
            sleep_until_next_frame(TARGET_FPS);
        }
        if (!running) break;

        // Меню игры
        bool in_game_menu = true;
        while (running && in_game_menu) {
            draw_game_menu(view, chosen_game);

            InputEvent_t ev = {0};
            res = cli_view.poll_input(view, &ev);
            if (res == VIEW_OK) {
                switch (ev.key_code) {
                    case '1':
                        // run_game_loop(chosen_game, view);
                        in_game_menu = false;
                        break;
                    case '9':
                        in_game_menu = false;
                        break;
                    case '0':
                        running = false;
                        in_game_menu = false;
                        break;
                    default:
                        break;
                }
            }
            sleep_until_next_frame(TARGET_FPS);
        }
    }

    // Проверка shutdown
    res = cli_view.shutdown(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "shutdown failed: %d\n", res);
        return 1;
    }

    return 0;
}