#include <unistd.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "snake.h"
#include "syn_buffer.h"
#include "pipe.h"
#include "../sockets-lib/socket.h"
#include "server.h"

time_t get_current_time() {
  return time(NULL);
}

void run_consumer(shared_names* names) {
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, names);
  
  int fd_passive = passive_socket_init(SOCKET_PC);
  int fd_active = passive_socket_wait_for_client(fd_passive);

  const int fd_pipe1 = pipe_open_read(names->pipe1);

  char game_mode;
  read(fd_pipe1, &game_mode, sizeof(char));
  char game_type;
  read(fd_pipe1, &game_type, sizeof(char));
  int game_dimension;
  read(fd_pipe1, &game_dimension, sizeof(int));
  double game_time;
  if (game_mode == 'c') {
    read(fd_pipe1, &game_time, sizeof(double));
  }

  game_world world;
  if (game_type == 'b') {
    init_game_without_obs(game_dimension, &world);
  } else {
    int n_obstacles = rand() % game_dimension + 1;
    init_game_with_obs(game_dimension, n_obstacles, &world);
  }

  write(fd_active, &world.size, sizeof(int));
  for (size_t i = 0; i < world.size; i++) {
    write(fd_active, world.field[i], world.size * sizeof(char));
  }

  char output;
  syn_shm_buffer_pop(&syn, &output);
  
  time_t start_time = get_current_time();

  atomic_bool isUserPlaying = true;
  while (isUserPlaying) {
    double time_passed = difftime(get_current_time(), start_time);
    bool moved = move_snake(&output, &world);

    if (output == 'k' || !moved || (game_mode == 'c' && time_passed >= game_time)) {
      isUserPlaying = false;
    }

    write(fd_active, &isUserPlaying, sizeof(atomic_bool));
    write(fd_active, &world.points, sizeof(int));
    write(fd_active, &time_passed, sizeof(double));
    
    write(fd_active, &game_mode, sizeof(char));
    if (game_mode == 'c') {
      write(fd_active, &game_time, sizeof(double));
    }

    if (output != 'k' && moved && (game_mode != 'c' || time_passed < game_time)) {
      for (size_t i = 0; i < world.size; i++) {
        write(fd_active, world.field[i], world.size * sizeof(char));
      }
      syn_shm_buffer_pop(&syn, &output);
    }
  }
  
  pipe_close(fd_pipe1);

  active_socket_destroy(fd_active);
  passive_socket_destroy(fd_passive);

  syn_shm_buffer_close(&syn);

  destroy_game(&world);
}

