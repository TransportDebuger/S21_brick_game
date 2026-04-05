#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "../gui/common/view.h"   // View_t, ViewHandle_t, ElementData_t, InputEvent_t
#include "../gui/cli/cli.h"       // cli_view extern const View_t
#include "s21_tetris.h"
#include "s21_snake.h"

#define STR_BUFFER_SIZE 1024
#define GAME_MENU(name) #name ":\n1 - Start game\n9 - Main menu\n0 - Exit\n"
#define GAMES_COUNT ((int)(sizeof(games) / sizeof(games[0])))
#define CAPTION_GAME_FIELD "MAIN BOARD"
#define CAPTION_SCORE "SCORE"
#define CAPTION_HISCORE "HI-SCORE"
#define CAPTION_NEXT_FIGURE "NEXT"
#define CAPTION_GAME_INFO "GAME INFO"

//structure to hold game metadata
typedef struct {
    int id;
    const char *name;
    const char *menu_text;
    GameInterface_t (*get_interface)(GameId_t);
    GameId_t game_id;
} GameInfo;


// Список доступных игр (id должен быть >0 и <=9, должен быть уникальным)
static const GameInfo games[] = {
    { .id = 1, .name = "Tetris", .menu_text = GAME_MENU(Tetris), .get_interface = tetris_get_interface, .game_id = GAME_TETRIS },
    { .id = 2, .name = "Snake", .menu_text = GAME_MENU(Snake), .get_interface = snake_get_interface, .game_id = GAME_SNAKE },
};


enum { TARGET_FPS = 30 };

static void draw_main_menu(ViewHandle_t view) {
    char buf[STR_BUFFER_SIZE];
    int offset = 0;
    int len = sizeof(buf);

    offset += snprintf(buf + offset, len - offset, "Main menu:\n");

    bool found = false;
    for (int i = 0; i < GAMES_COUNT; ++i) {
        if (games[i].id >= 1 && games[i].id <= 9) {
            int written = snprintf(buf + offset, len - offset, "%d - %s\n",
                                   games[i].id, games[i].name);
            if (written < 0) {
                fprintf(stderr, "snprintf error in menu generation\n");
                return;
            }
            if (written >= len - offset) {
                fprintf(stderr, "Buffer overflow in main menu text\n");
                break;
            }
            offset += written;
            found = true;
        }
    }

    if (!found) {
        offset += snprintf(buf + offset, len - offset, "No games available\n");
    }

    snprintf(buf + offset, len - offset, "0 - Exit\n");

    ElementData_t data = {
        .type = ELEMENT_TEXT,
        .content.text = buf
    };

    ViewResult_t res = cli_view.draw_element(view, CAPTION_GAME_FIELD, &data);
    if (res != VIEW_OK) {
        fprintf(stderr, "draw_element failed: %d\n", res);
        return;
    }

    res = cli_view.render(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "render failed: %d\n", res);
    }
}

static const GameInfo* get_game_info(int game_id) {
    for (int i = 0; i < GAMES_COUNT; ++i) {
        if (games[i].id == game_id)
            return &games[i];
    }
    return NULL;
}

static const char* get_game_menu_text(int game_id) {
    const GameInfo* game = get_game_info(game_id);
    return game ? game->menu_text : "Unknown game\n9 - Back\n0 - Exit\n";
}

static void draw_game_menu(ViewHandle_t view, int game_id) {
    char menu_text[STR_BUFFER_SIZE];
    const char *template = get_game_menu_text(game_id);
    snprintf(menu_text, sizeof(menu_text), template, game_id);
    ElementData_t data = { .type = ELEMENT_TEXT, .content.text = menu_text };

    ViewResult_t res = cli_view.draw_element(view, CAPTION_GAME_FIELD, &data);
    if (res != VIEW_OK) {
        fprintf(stderr, "draw_element failed: %d\n", res);
        return;
    }

    res = cli_view.render(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "render failed: %d\n", res);
    }
}

void sleep_until_next_frame(int fps) {
    const long frame_ns = 1000000000L / fps;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = frame_ns };
    nanosleep(&ts, NULL);
}

bool handle_menu_input(ViewHandle_t view, const ViewInterface *v,
                       int *chosen_game, bool *running, bool *in_game_menu) {
    InputEvent_t ev = {0};
    ViewResult_t res = v->poll_input(view, &ev);

    if (res != VIEW_OK && res != VIEW_NO_EVENT) {
        return false;
    }

    // Handle digits from '1' to '9'
    if (ev.key_code >= '1' && ev.key_code <= '9' && *in_game_menu == false) {
        int id = ev.key_code - '0';

        // Check if a game with that id exists
        const GameInfo* game = get_game_info(id);
        if (game) {
            *chosen_game = id;
            return true;
        }
    }

    // Special keys
    switch (ev.key_code) {
        case '0':
            *running = false;
            return true;
        case '9':
            *in_game_menu = false;
            *chosen_game = -1;
            return true;
        case '1':
            if (*in_game_menu) {
                *in_game_menu = false;
                return true;
            }
            return false;
        default:
            return false;
    }
}

int main(void) {
    const int field_w = BGAME_FIELD_WIDTH;
    const int field_h = BGAME_FIELD_HEIGHT;

    ViewHandle_t view = cli_view.init(field_w, field_h, TARGET_FPS);
    if (!view) {
        fprintf(stderr, "Failed to init CLI view\n");
        return 1;
    }

    // Register all games with the framework
    for (int i = 0; i < GAMES_COUNT; ++i) {
        GameInterface_t iface = games[i].get_interface(games[i].game_id);
        bg_register_game(iface);
    }

    // Configure playable zone
    ViewResult_t res = cli_view.configure_zone(view, CAPTION_GAME_FIELD, 2, 2, BGAME_FIELD_WIDTH * 2, BGAME_FIELD_HEIGHT);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }

    // Configure zones for game info
    res = cli_view.configure_zone(view, CAPTION_SCORE, 25, 2, 15, 1);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }
    res = cli_view.configure_zone(view, CAPTION_HISCORE, 25, 5, 15, 1);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }
    res = cli_view.configure_zone(view, CAPTION_NEXT_FIGURE, 25, 8, 15, 4);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }
    res = cli_view.configure_zone(view, CAPTION_GAME_INFO, 25, 14, 15, 8);
    if (res != VIEW_OK) {
        fprintf(stderr, "configure_zone failed: %d\n", res);
        cli_view.shutdown(view);
        return 1;
    }

    bool running = true;
    while (running) {
        int chosen_game = -1;
        bool in_game_menu = false;

        // Main menu
        while (running && chosen_game == -1) {
            draw_main_menu(view);
            handle_menu_input(view, &cli_view, &chosen_game, &running, &in_game_menu);
            sleep_until_next_frame(TARGET_FPS);
        }
        if (!running) break;

        const GameInfo* selected_game = get_game_info(chosen_game);
        if (!selected_game) continue;

        // Switch to the selected game
        if (!bg_switch_game(selected_game->game_id)) {
            fprintf(stderr, "Failed to switch to game: %s\n", selected_game->name);
            continue;
        }

        in_game_menu = true;
        // Game menu
        while (running && in_game_menu) {
            draw_game_menu(view, chosen_game);
            handle_menu_input(view, &cli_view, &chosen_game, &running, &in_game_menu);
            sleep_until_next_frame(TARGET_FPS);
        }
        if (!in_game_menu && chosen_game == -1) continue;

        // Main game loop
        while (running && bg_get_current_instance()) {
            InputEvent_t ev = {0};
            ViewResult_t res = cli_view.poll_input(view, &ev);

            if (res == VIEW_OK) {
                UserAction_t action = Start;
                bool hold = false;
                if (ev.key_state == 1) hold = true;

                switch (ev.key_code) {
                    case 'w': case 'W': action = Up; break;
                    case 'a': case 'A': action = Left; break;
                    case 's': case 'S': action = Down; break;
                    case 'd': case 'D': action = Right; break;
                    case ' ': action = Action; break;
                    case 'p': case 'P': action = Pause; break;
                    case 'q': case 'Q': action = Terminate; break;
                    default: break;
                }
                userInput(action, hold);
            } else if (res != VIEW_NO_EVENT) {
                fprintf(stderr, "poll_input failed: %d\n", res);
                break;
            }

            GameInfo_t info = updateCurrentState();

            // Draw game field
            ElementData_t field_data = {
                .type = ELEMENT_MATRIX,
                .content.matrix = {
                    .data = (const int*)info.field,
                    .width = BGAME_FIELD_WIDTH,
                    .height = BGAME_FIELD_HEIGHT
                }
            };
            res = cli_view.draw_element(view, CAPTION_GAME_FIELD, &field_data);
            if (res != VIEW_OK) {
                fprintf(stderr, "draw_element failed: %d\n", res);
                break;
            }

            // Draw score
            ElementData_t score_data = {
                .type = ELEMENT_NUMBER,
                .content.number = info.score
            };
            res = cli_view.draw_element(view, CAPTION_SCORE, &score_data);
            if (res != VIEW_OK) {
                fprintf(stderr, "draw_element failed: %d\n", res);
                break;
            }

            // Draw high score
            ElementData_t hiscore_data = {
                .type = ELEMENT_NUMBER,
                .content.number = info.high_score
            };
            res = cli_view.draw_element(view, CAPTION_HISCORE, &hiscore_data);
            if (res != VIEW_OK) {
                fprintf(stderr, "draw_element failed: %d\n", res);
                break;
            }

            // Draw next figure
            ElementData_t next_data = {
                .type = ELEMENT_MATRIX,
                .content.matrix = {
                    .data = (const int*)info.next,
                    .width = 4,
                    .height = 4
                }
            };
            res = cli_view.draw_element(view, CAPTION_NEXT_FIGURE, &next_data);
            if (res != VIEW_OK) {
                fprintf(stderr, "draw_element failed: %d\n", res);
                break;
            }

            // Draw game info (level, speed, pause)
            char info_text[256];
            snprintf(info_text, sizeof(info_text), "Level: %d\nSpeed: %d\nPause: %s",
                    info.level, info.speed, info.pause ? "Yes" : "No");
            ElementData_t info_data = {
                .type = ELEMENT_TEXT,
                .content.text = info_text
            };
            res = cli_view.draw_element(view, CAPTION_GAME_INFO, &info_data);
            if (res != VIEW_OK) {
                fprintf(stderr, "draw_element failed: %d\n", res);
                break;
            }

            // Render all
            res = cli_view.render(view);
            if (res != VIEW_OK) {
                fprintf(stderr, "render failed: %d\n", res);
                break;
            }

            sleep_until_next_frame(info.speed > 0 ? info.speed : TARGET_FPS);
        }
    }

    res = cli_view.shutdown(view);
    if (res != VIEW_OK) {
        fprintf(stderr, "shutdown failed: %d\n", res);
        return 1;
    }

    return 0;
}