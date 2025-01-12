#ifndef SNAKE
#define SNAKE

#include <stdbool.h>

typedef struct position {
  int x;
  int y;
} position;

typedef struct snake {
  int len;
  position* body;
} snake;

typedef struct game_world {
  int size;
  char** field;
  snake snake;
  int points;
} game_world;

void init_game_without_obs(int size_, game_world* world);
void init_game_with_obs(int size_, int n_obstacles, game_world* world);
void init_field(game_world* world);
void destroy_field(game_world* world);
void generate_fruit(game_world* world);
void generate_obstacles(game_world* world, int n_obstacles);
void destroy_game(game_world* world);
void print_world(game_world* world);
bool move_snake(char* direction, game_world* world);
int check_collisions(position* new_pos, game_world* world);


#endif
