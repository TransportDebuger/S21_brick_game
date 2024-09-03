#ifndef GAMEPREF_H
#define GAMEPREF_H

/*
    Ширирна игрового поля согласно условиям задания.
*/
#define GAME_BOARD_WIDTH 10

/*
    Высота игрового поля включая дополнительные (невидимые)
    ячейки для работы с новыми фигурами.
*/
#define GAME_BOARD_HEIGHT 22

/*
    Видимая высота игрового поля согласно условий задачи
*/
#define GAME_BOARD_VISIBLE_HEIGHT = 20
/*
    Задержка выполнения цикла
*/
#define GAME_SPEED_DELAY 1000
/*
    Модификатор задержки для изменения скорости в игре
*/
#define GAME_SPEED_DELAY_MODIFIER 0.9

#endif