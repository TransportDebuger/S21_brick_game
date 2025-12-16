/**
 * @file s21_bgame_cmn.c
 * @brief Реализация общих утилит BrickGame
 *
 * Реализует все функции утилит, объявленные в brickgame_common.h:
 * - Выделение и освобождение памяти для игрового поля и превью
 * - Сохранение и загрузка рекордов (хранение на файловой системе)
 * - Инициализация и очистка структуры GameInfo_t
 * - Валидация пользовательского ввода и данных
 *
 * @author BrickGame Project
 * @version 1.0
 * @date 2025-12-16
 */

#include "s21_bgame_cmn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>

#define SCORE_DIR ".brickgame"
#define SCORE_FILENAME_MAX 512

int** brickgame_allocate_field(void) {
    int **field = malloc(FIELD_HEIGHT * sizeof(int*));
    if (field == NULL) {
        return NULL;
    }

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        field[i] = malloc(FIELD_WIDTH  * sizeof(int));
        if (field[i] == NULL) {
            // Cleanup on allocation failure
            for (int j = 0; j < i; j++) {
                free(field[j]);
            }
            free(field);
            return NULL;
        }
        // Initialize to 0 (empty)
        memset(field[i], 0, FIELD_WIDTH  * sizeof(int));
    }

    return field;
}

void brickgame_free_field(int **field) {
    if (field == NULL) {
        return;
    }

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        free(field[i]);
    }
    free(field);
}

void brickgame_clear_field(int **field) {
    if (field == NULL) {
        return;
    }

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH ; j++) {
            field[i][j] = 0;
        }
    }
}

int** brickgame_allocate_next(void) {
    int **next = malloc(PREVIEW_SIZE * sizeof(int*));
    if (next == NULL) {
        return NULL;
    }

    for (int i = 0; i < PREVIEW_SIZE; i++) {
        next[i] = malloc(PREVIEW_SIZE * sizeof(int));
        if (next[i] == NULL) {
            // Cleanup on allocation failure
            for (int j = 0; j < i; j++) {
                free(next[j]);
            }
            free(next);
            return NULL;
        }
        // Initialize to 0 (empty)
        memset(next[i], 0, PREVIEW_SIZE * sizeof(int));
    }

    return next;
}

void brickgame_free_next(int **next) {
    if (next == NULL) {
        return;
    }

    for (int i = 0; i < PREVIEW_SIZE; i++) {
        free(next[i]);
    }
    free(next);
}

void brickgame_clear_next(int **next) {
    if (next == NULL) {
        return;
    }

    for (int i = 0; i < PREVIEW_SIZE; i++) {
        for (int j = 0; j < PREVIEW_SIZE; j++) {
            next[i][j] = 0;
        }
    }
}

/**
 * @brief Получить путь к файлу для сохранения рекорда
 *
 * Строит и возвращает полный путь, где будет сохранён рекорд игры.
 * Путь следует соглашению:
 * ~/.brickgame/<имя_игры>.score
 *
 * Внутренняя функция, используется функциями brickgame_load_high_score()
 * и brickgame_save_high_score().
 *
 * @param game_name Имя игры (например, "tetris", "snake")
 * @param buffer    Выходной буфер для сохранения пути
 * @param size      Размер буфера
 *
 * @return Указатель на buffer при успехе, или NULL при ошибке
 *
 * @note Вызывающая сторона должна предоставить буфер размером минимум 256 байт
 * @note Если буфер слишком мал, возвращает NULL и оставляет буфер без изменений
 *
 * @internal
 */
 static char* brickgame_get_score_path(const char *game_name, char *buffer,
                               size_t size) {
    if (game_name == NULL || buffer == NULL || size < 256) {
        return NULL;
    }

    // Get home directory
    const char *home = getenv("HOME");
    if (home == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL) {
            return NULL;
        }
        home = pw->pw_dir;
    }

    // Create .brickgame directory if it doesn't exist
    int snprintf_result = snprintf(buffer, size, "%s/%s", home, SCORE_DIR);
    if (snprintf_result < 0 || (size_t)snprintf_result >= size) {
        return NULL;
    }

    // Attempt to create directory (will fail silently if it exists)
    mkdir(buffer, 0755);

    // Create full path to score file
    snprintf_result = snprintf(buffer, size, "%s/%s/%s.score", home,
                               SCORE_DIR, game_name);
    if (snprintf_result < 0 || (size_t)snprintf_result >= size) {
        return NULL;
    }

    return buffer;
}

int brickgame_load_high_score(const char *game_name) {
    if (game_name == NULL) {
        return 0;
    }

    char path[SCORE_FILENAME_MAX];
    if (brickgame_get_score_path(game_name, path, sizeof(path)) == NULL) {
        return 0;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // File doesn't exist (first run) - not an error
        return 0;
    }

    int score = 0;
    int read_result = fscanf(file, "%d", &score);
    fclose(file);

    if (read_result != 1 || score < 0) {
        // File corrupted or invalid score
        return 0;
    }

    return score;
}

bool brickgame_save_high_score(const char *game_name, int score) {
    if (game_name == NULL || score < 0) {
        return false;
    }

    char path[SCORE_FILENAME_MAX];
    if (brickgame_get_score_path(game_name, path, sizeof(path)) == NULL) {
        return false;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror("brickgame_save_high_score: fopen");
        return false;
    }

    int write_result = fprintf(file, "%d", score);
    fclose(file);

    if (write_result < 0) {
        return false;
    }

    return true;
}

GameInfo_t brickgame_create_game_info(void) {
    GameInfo_t info = {0};

    info.field = brickgame_allocate_field();
    info.next = brickgame_allocate_next();
    info.score = 0;
    info.high_score = 0;
    info.level = MIN_LEVEL;
    info.speed = 800;
    info.pause = 0;

    return info;
}

void brickgame_destroy_game_info(GameInfo_t *info) {
    if (info == NULL) {
        return;
    }

    brickgame_free_field(info->field);
    brickgame_free_next(info->next);

    // Zero out the structure
    memset(info, 0, sizeof(GameInfo_t));
}

bool brickgame_is_valid_action(UserAction_t action) {
    return action >= Start && action <= Action;
}

bool brickgame_is_valid_field(int **field) {
    if (field == NULL) {
        return false;
    }

    // Check all rows and columns
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        if (field[i] == NULL) {
            return false;
        }

        for (int j = 0; j < FIELD_WIDTH ; j++) {
            if (field[i][j] < 0 || field[i][j] > 1) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Проверить корректность массива превью следующей фигуры
 *
 * Проверяет следующее:
 * - Указатель next не является NULL
 * - Массив next правильно выделен (4 строки x 4 столбца)
 * - Каждая ячейка содержит допустимое значение (0 или 1)
 *
 * @param next Массив превью для проверки
 *
 * @return true если превью корректно, false иначе
 *
 * @note Это внутренняя утилита, используется функцией brickgame_is_valid_game_info()
 *
 * @internal
 */
bool brickgame_is_valid_next(int **next) {
    if (next == NULL) {
        return false;
    }

    // Check all rows and columns
    for (int i = 0; i < PREVIEW_SIZE; i++) {
        if (next[i] == NULL) {
            return false;
        }

        for (int j = 0; j < PREVIEW_SIZE; j++) {
            // Each cell must be 0 (empty) or 1 (filled)
            if (next[i][j] < 0 || next[i][j] > 1) {
                return false;
            }
        }
    }

    return true;
}

bool brickgame_is_valid_game_info(const GameInfo_t *info) {
    if (info == NULL) {
        return false;
    }

    // Check pointers
    if (!brickgame_is_valid_field(info->field)) {
        return false;
    }

    if (!brickgame_is_valid_next(info->next)) {
        return false;
    }

    // Check numeric fields
    if (info->score < 0 || info->high_score < 0) {
        return false;
    }

    if (info->level < MIN_LEVEL || info->level > MAX_LEVEL) {
        return false;
    }

    if (info->speed < 0) {
        return false;
    }

    if (info->pause < 0 || info->pause > 1) {
        return false;
    }

    return true;
}