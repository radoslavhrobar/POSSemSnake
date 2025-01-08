#include "snake.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void init_game_without_obs(int size_, game_world* world) {
  world->size = size_;
  world->snake.len = 1;
  world->snake.body = malloc(world->snake.len * sizeof(position));
  world->snake.body[0] = (position){size_ / 2, size_ / 2};
  init_field(world);
  for (int i = 0; i < world->size; i++) {
    for (int j = 0; j < world->size; j++) {
        world->field[i][j] = '-'; 
    }
  }
  world->field[world->snake.body[0].y][world->snake.body[0].x] = '*';
  generate_fruit(world);
}


void init_field(game_world* world) {
  world->field = malloc(world->size * sizeof(char*));
  for (int i = 0; i < world->size; i++) {
    world->field[i] = malloc(world->size * sizeof(char));
  }
}


void init_game_with_obs(int size, int n_obstacles, game_world* world) {
  init_game_without_obs(size, world);
  generate_obstacles(world, n_obstacles);
}

void destroy_game(game_world* world) {
  free(world->snake.body);
  for (size_t i = 0; i < world->size; i++) {
    free(world->field[i]);
  }
  free(world->field);
}

void generate_fruit(game_world* world) {
  int x, y;
  do {
    x = rand() % world->size;
    y = rand() % world->size;
  } while (world->field[y][x] != '-'); 
  world->field[y][x] = 'F';
}

void generate_obstacles(game_world* world, int n_obstacles) {
  int x, y;
  for (size_t i = 0; i < n_obstacles; i++) {
    do {
      x = rand() % world->size;
      y = rand() % world->size;
    } while (world->field[y][x] != '-');
    world->field[y][x] = 'O';
  }
}

void print_world(game_world* world) {
  for (size_t i = 0; i < world->size; i++) {
    for (size_t j = 0; j < world->size; j++) {
      printf("%c", world->field[i][j]);
    }
    printf("\n");
  }
}

bool move_snake(char* direction, game_world* world) {
  position new_pos = world->snake.body[world->snake.len-1];
  switch (*direction) {
    case 'w':
      new_pos.y--;
      break;
    case 's':
      new_pos.y++;
      break;
    case 'a':
      new_pos.x--;
      break;
    case 'd':
      new_pos.x++;
      break;
  }
  int kontrola = check_collisions(&new_pos, world);
  
  if (kontrola == 1) {
    return false;
  }
  
  world->field[world->snake.body[0].y][world->snake.body[0].x] = '-';
  position prev_pos = world->snake.body[0];

  for (size_t i = 0; i < world->snake.len - 1; i++) {
    world->snake.body[i] = world->snake.body[i+1];
  }

  world->snake.body[world->snake.len-1] = new_pos;
  world->field[new_pos.y][new_pos.x] = '*';
  
  if (kontrola == 0) {
    world->snake.len++;
    world->snake.body = realloc(world->snake.body, world->snake.len * sizeof(position));
    
    for (size_t i = world->snake.len - 1; i > 0; i--) {
      world->snake.body[i] = world->snake.body[i-1];
    }
    world->snake.body[0] = prev_pos;
    world->field[prev_pos.y][prev_pos.x] = '*';
    generate_fruit(world);
  }
  return true;
}

int check_collisions(position* new_pos, game_world* world) {
  if (new_pos->x == world->size) {
    new_pos->x = 0;
  } else if (new_pos->x == -1) {
    new_pos->x = world->size-1;
  } else if (new_pos->y == world->size) {
    new_pos->y = 0;
  } else if (new_pos->y == -1) {
    new_pos->y = world->size-1;
  }

  if (world->field[new_pos->y][new_pos->x] == 'F') {
    return 0;                                                                           
  } else if (world->field[new_pos->y][new_pos->x] == 'O') {
    return 1;
  } else if (world->field[new_pos->y][new_pos->x] == '*') {
    return 1;
  }
  return 2;
}




