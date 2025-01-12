#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "snake.h"
#include "syn_buffer.h"
#include "pipe.h"
#include "../sockets-lib/socket.h"
#include "client.h"
#include "server.h"


void print_basic_info() {
  printf("Instrukcie ku hre:\n");
  printf("Pohyb:\n");
  printf("w - hore | a - vlavo | s - dole | d - vpravo\n");
  printf("Ostatne:\n");
  printf("p - pauza | k - koniec\n");
}

char get_first_input() {
  char input;
  scanf(" %c", &input);
  while (input != 'w' && input != 'a' && input != 's' && input != 'd' && input != 'k') {
    if (input == 'p') {
      printf("Nemozes zastavit nespustenu hru!\n");
    } else {
      printf("Nespravny vyber cinnosti!\n");
      print_basic_info();
    }
    scanf(" %c", &input);
  } 
  return input;

}

char get_user_input() {
  char input;
  scanf(" %c", &input);
  while (input != 'w' && input != 'a' && input != 's' && input != 'd' && input != 'p' && input != 'k') {
    scanf(" %c", &input);
  } 
  return input;
}

char get_user_input_pause() {
  char input;
  scanf(" %c", &input);
  while (input != 'p' && input != 'k') {
    printf("Nespravny vyber cinnosti!\n");
    printf("p - pauza | k - koniec\n");
    scanf(" %c", &input);
  } 
  return input;
}

char get_game_mode() {
  char input;
  printf("Zadaj herny rezim -> [s] standardny | [c] casovy\n");
  scanf(" %c", &input);
  while (input != 's' && input != 'c') {
    printf("Nespravny herny rezim! -> [s] standardny | [c] casovy\n");
    scanf(" %c", &input); 
  }
  return input;
}

char get_game_type() {
  char input;
  printf("Zadaj typ herneho sveta -> [b] bez prekazok | [p] prekazky\n");
  scanf(" %c", &input);
  while (input != 'b' && input != 'p') {
    printf("Nespravny typ herneho sveta! -> [b] bez prekazok | [p] prekazky\n");
    scanf(" %c", &input);
  }
  return input;
}

int get_dimension() {
  int input;
  printf("Zadaj rozmer herneho sveta od 10 do 30\n");
  scanf(" %d", &input);
  while (input < 10 || input > 30) {
    printf("Rozmer moze byt len od 10 do 30!\n");
    scanf(" %d", &input);
  }
  return input;
}

double get_game_time() {
  double input;
  printf("Zadaj cas na hru\n");
  scanf(" %lf", &input);
  return input;
}

void * run_producer_output(void* arg) {
  producer_data* data = arg;
  int fd_active = connect_to_server("localhost", SOCKET_PC);
  game_world world;
  read(fd_active, &world.size, sizeof(world.size));

  init_field(&world);
  for (size_t i = 0; i < world.size; i++) {
    read(fd_active, world.field[i], world.size * sizeof(char));
  }
  print_world(&world);
  print_basic_info();
  printf("HRU SPUSTIS ZADANIM POHYBU\n");

  int points;
  double time_passed;
  char game_mode;
  double game_time;
  while (data->isUserPlaying) {
    read(fd_active, &data->isUserPlaying, sizeof(atomic_bool));
    read(fd_active, &points, sizeof(int));
    read(fd_active, &time_passed, sizeof(double));
    read(fd_active, &game_mode, sizeof(char));

    if (game_mode == 'c') {
      read(fd_active, &game_time, sizeof(double));
    }

    printf("Pocet nahratych bodov: %d, straveny cas: %.0lf sekund\n", points, time_passed);
    if (data->isUserPlaying) {
      for (size_t i = 0; i < world.size; i++) {
        read(fd_active, world.field[i], world.size * sizeof(char));
      }
      print_world(&world);
    } else {
      if (game_mode == 'c') {
        if (time_passed >= game_time) {
          printf("Cas vyprsal.\n");
        } else {
          printf("Hra ukoncena.\n");
        }
      } else {
          printf("Hra ukoncena.\n");
      }
    }
  }
  destroy_field(&world);

  active_socket_destroy(fd_active);
  return NULL;
}

void* run_producer_input(void* arg) {
  producer_data* data = arg;
  while (data->isUserPlaying) {
    char prev = data->input;
    data->input = get_user_input();
    if (data->input == 'p') {
      data->paused = true;
      printf("Pozastavil si hru. Chces [p] pokracovat alebo [k] ukoncit hru?\n");
      print_basic_info();
      while (data->paused) {
        data->input = get_user_input_pause();
        if (data->input == 'p' || data->input == 'k') {
          if (data->input == 'p') {
            data->input = prev;
            sleep(3);
          }
          data->paused = false;
        }
      }
    }
  }
  return NULL;
}

void* run_producer_input_push(void* arg) {
  producer_data* data = arg;
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, data->names);

  const int fd_pipe1 = pipe_open_write(data->names->pipe1);

  char game_mode = get_game_mode();
  write(fd_pipe1, &game_mode, sizeof(char));
  char game_type = get_game_type();
  write(fd_pipe1, &game_type, sizeof(char));
  int game_dimension = get_dimension();
  write(fd_pipe1, &game_dimension, sizeof(int));
  if (game_mode == 'c') {
    double game_time = get_game_time();
    write(fd_pipe1, &game_time, sizeof(double));
  }
  
  data->input = get_first_input();
  
  pthread_t t_input;
  pthread_create(&t_input, NULL, run_producer_input, data);

  while (data->isUserPlaying) {
    if (data->input == 'p') {
      while (data->paused) {
        usleep(100000);
      }
    }
    syn_shm_buffer_push(&syn, &data->input);
    usleep(500000);
  }

  pthread_join(t_input, NULL);

  pipe_close(fd_pipe1);

  syn_shm_buffer_close(&syn);
  return NULL;
}

void dispatch_producer(shared_names* names, char in, atomic_bool isUserPlaying, atomic_bool paused) {
  producer_data data = {names, in, isUserPlaying, paused};
  pthread_t input_push;
  pthread_t output;
  pthread_create(&input_push, NULL, run_producer_input_push, &data);
  pthread_create(&output, NULL, run_producer_output, &data);
  pthread_join(input_push, NULL);
  pthread_join(output, NULL);
}

