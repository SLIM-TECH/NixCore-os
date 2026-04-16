
#ifndef GAMES_H
#define GAMES_H

void game_snake(void);
void game_matrix(void);
void game_pianino(void);

void snake_draw_game(void);
void snake_move(void);
int snake_check_collision(void);
void snake_place_food(void);

#endif
