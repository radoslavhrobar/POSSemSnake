#include "snake.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void init_game_without_obs(int size_, game_world* world) {
  world->size = size_;
  world->snake.len = 3;
  world->snake.body = malloc(world->snake.len * sizeof(position));
  for (size_t i = 0; i < world->snake.len; i++) {
    world->snake.body[i] = (position){size_ / 2, size_ / 2 - i};
  }
  init_field(world);
  for (int i = 0; i < world->size; i++) {
    for (int j = 0; j < world->size; j++) {
        world->field[i][j] = ' '; 
    }
  }
  for (size_t i = 0; i < world->snake.len; i++) {
    world->field[world->snake.body[i].y][world->snake.body[i].x] = 'O';
  }
  generate_fruit(world);
  world->points = 0;
}


void init_field(game_world* world) {
  world->field = malloc(world->size * sizeof(char*));
  for (int i = 0; i < world->size; i++) {
    world->field[i] = malloc(world->size * sizeof(char));
  }
}

void destroy_field(game_world* world) {
  for (size_t i = 0; i < world->size; i++) {
    free(world->field[i]);
  }
  free(world->field);
}


void init_game_with_obs(int size, int n_obstacles, game_world* world) {
  init_game_without_obs(size, world);
  generate_obstacles(world, n_obstacles);
}

void destroy_game(game_world* world) {
  free(world->snake.body);
  destroy_field(world);
}

void generate_fruit(game_world* world) {
  int x, y;
  do {
    x = rand() % world->size;
    y = rand() % world->size;
  } while (world->field[y][x] != ' '); 
  world->field[y][x] = 'F';
}

void generate_obstacles(game_world* world, int n_obstacles) {
  int x, y;
  for (size_t i = 0; i < n_obstacles; i++) {
    do {
      x = rand() % world->size;
      y = rand() % world->size;
    } while (world->field[y][x] != ' ');
    world->field[y][x] = '+';
  }
}

void print_world(game_world* world) {
  for (size_t i = 0; i < world->size+2; i++) {
    printf("-");
  }
  printf("\n");
  for (size_t i = 0; i < world->size; i++) {
    printf("|");
    for (size_t j = 0; j < world->size; j++) {
      printf("%c", world->field[i][j]);
    }
    printf("|\n");
  }
  for (size_t i = 0; i < world->size+2; i++) {
    printf("-");
  }
  printf("\n");
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
  
  world->field[world->snake.body[0].y][world->snake.body[0].x] = ' ';
  position prev_pos = world->snake.body[0];

  for (size_t i = 0; i < world->snake.len - 1; i++) {
    world->snake.body[i] = world->snake.body[i+1];
  }

  world->snake.body[world->snake.len-1] = new_pos;
  world->field[new_pos.y][new_pos.x] = 'O';
  
  if (kontrola == 0) {
    world->points++;
    world->snake.len++;
    world->snake.body = realloc(world->snake.body, world->snake.len * sizeof(position));
    
    for (size_t i = world->snake.len - 1; i > 0; i--) {
      world->snake.body[i] = world->snake.body[i-1];
    }
    world->snake.body[0] = prev_pos;
    world->field[prev_pos.y][prev_pos.x] = 'O';
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
  } else if (world->field[new_pos->y][new_pos->x] == '+') {
    return 1;
  }
  return 2;
}




