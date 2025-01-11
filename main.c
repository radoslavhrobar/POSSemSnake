#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "syn_buffer.h"
#include "pipe.c"
#include "snake.h"
#include "../sockets-lib/socket.h"

#define SOCKET_PC 4921

char get_user_input() {
  char input;
  printf("Zadaj cinnost -> [w] hore | [a] vlavo | [s] dole | [d] vpravo | [p] pauza | [k] koniec\n");
  scanf(" %c", &input);
  while (input != 'w' && input != 'a' && input != 's' && input != 'd' && input != 'p' && input != 'k') {
    printf("Nesprávny výber činnosti!\n");
    printf("Zadaj cinnost -> [w] hore | [a] vlavo | [s] dole | [d] vpravo | [p] pauza | [k] koniec\n");
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
  printf("Zadaj rozmer herneho sveta od 10 do 40\n");
  scanf(" %d", &input);
  while (input < 10 || input > 40) {
    printf("Rozmer moze byt len od 10 do 40!\n");
    scanf(" %d", &input);
  }
  return input;
}

time_t get_current_time() {
  return time(NULL);
}

double get_game_time() {
  double input;
  printf("Zadaj cas na hru\n");
  scanf(" %lf", &input);
  return input;

}

void run_consumer(shared_names* names) {
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, names);
  
  int fd_passive = passive_socket_init(SOCKET_PC);
  int fd_active = passive_socket_wait_for_client(fd_passive);

  const int fd_pipe1 = pipe_open_read(names->pipe1);
  const int fd_pipe2 = pipe_open_write(names->pipe2);


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
    int n_obstacles = rand() % 10 + 1;
    init_game_with_obs(game_dimension, n_obstacles, &world);
  }

  write(fd_active, &world.size, sizeof(int));
  for (size_t i = 0; i < world.size; i++) {
    write(fd_active, world.field[i], world.size * sizeof(char));
  }

  char output;
  syn_shm_buffer_pop(&syn, &output);
  
  time_t start_time = get_current_time();

  bool isUserPlaying = true;
  while (isUserPlaying) {
    double time_passed = difftime(get_current_time(), start_time);
    if (!move_snake(&output, &world) || (game_mode == 'c' && time_passed == game_time)) {
      isUserPlaying = false;
      write(fd_active, &isUserPlaying, sizeof(bool));
      write(fd_active, &world.points, sizeof(int));
      write(fd_active, &time_passed, sizeof(double));
      write(fd_active, &game_mode, sizeof(char));
      if (game_mode == 'c') {
         write(fd_active, &game_time, sizeof(double));
      }
      write(fd_pipe2, &isUserPlaying, sizeof(bool));
    } else {
      write(fd_active, &isUserPlaying, sizeof(bool));
      write(fd_pipe2, &isUserPlaying, sizeof(bool));

      for (size_t i = 0; i < world.size; i++) {
        write(fd_active, world.field[i], world.size * sizeof(char));
      }
      syn_shm_buffer_pop(&syn, &output);
    }
  }
  
  pipe_close(fd_pipe1);
  pipe_close(fd_pipe2);

  active_socket_destroy(fd_active);
  passive_socket_destroy(fd_passive);

  syn_shm_buffer_close(&syn);

  destroy_game(&world);
}

typedef struct producer_data {
  shared_names* names;
  char input;
  atomic_bool isUserPlaying;
} producer_data;

void * run_producer_output(void* arg) {
  int fd_active = connect_to_server("localhost", SOCKET_PC);
  game_world world;
  read(fd_active, &world.size, sizeof(world.size));

  init_field(&world);
  for (size_t i = 0; i < world.size; i++) {
    read(fd_active, world.field[i], world.size * sizeof(char));
  }
  print_world(&world);

  bool isUserPlaying = true;
  while (isUserPlaying) {
    read(fd_active, &isUserPlaying, sizeof(bool));
    if (isUserPlaying) {
      for (size_t i = 0; i < world.size; i++) {
        read(fd_active, world.field[i], world.size * sizeof(char));
      }
      print_world(&world);
    } else {
      int points;
      double time_passed;
      double game_mode;
      read(fd_active, &points, sizeof(int));
      read(fd_active, &time_passed, sizeof(double));
      read(fd_active, &game_mode, sizeof(char));
      if (game_mode == 'c') {
        double game_time;
        read(fd_active, &game_time, sizeof(double));
        if (game_time == time_passed) {
          printf("Cas vyprsal.\n");
        } else {
          printf("Hra ukončená.\n");
        }
      } else {
        printf("Hra ukončená.\n");
      }
      printf("pocet nahratych bodov: %d, straveny cas: %.2lf sekundy\n", points, time_passed);
      }
  }
  
  active_socket_destroy(fd_active);
  return NULL;
}

void* run_producer_input(void* arg) {
  producer_data* data = arg;
  while (data->isUserPlaying) {
    data->input = get_user_input();
  }
  return NULL;
}

void* run_producer_input_push(void* arg) {
  producer_data* data = arg;
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, data->names);

  const int fd_pipe1 = pipe_open_write(data->names->pipe1);
  const int fd_pipe2 = pipe_open_read(data->names->pipe2);


  char game_mode = get_game_mode();
  write(fd_pipe1, &game_mode, sizeof(char));
  char game_type = get_game_type();
  write(fd_pipe1, &game_type, sizeof(char));
  int game_dimension = get_dimension();
  write(fd_pipe1, &game_dimension, sizeof(int));
  if (game_mode == 'c') {
    double time = get_game_time();
    write(fd_pipe1, &time, sizeof(double));
  }
  
  data->input = get_user_input();
  
  pthread_t t_input;
  pthread_create(&t_input, NULL, run_producer_input, data);

  while (data->isUserPlaying) {
    syn_shm_buffer_push(&syn, &data->input);
    read(fd_pipe2, &data->isUserPlaying, sizeof(bool));
    sleep(2);
  }

  pthread_join(t_input, NULL);

  pipe_close(fd_pipe1);
  pipe_close(fd_pipe2);

  syn_shm_buffer_close(&syn);
  return NULL;
}

void dispatch_producer(shared_names* names, char in, atomic_bool isUserPlaying) {
  producer_data data = {names, in, isUserPlaying};
  pthread_t input_push;
  pthread_t output;
  pthread_create(&input_push, NULL, run_producer_input_push, &data);
  pthread_create(&output, NULL, run_producer_output, &data);
  pthread_join(input_push, NULL);
  pthread_join(output, NULL);
}

char * add_suffix(const char * name, const char * suffix) {
  char * result = calloc(strlen(name) + strlen(suffix) + 2, sizeof(char));
  strcpy(result, name);
  result[strlen(name)] = '-';
  strcpy(result + strlen(name) + 1, suffix);
  return result;
}


void destroy_names(shared_names * names) {
  free(names->shm_name_);
  free(names->mut_pc_);
  free(names->sem_produce_);
  free(names->sem_consume_);
  free(names->pipe1);
  free(names->pipe2);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Nezadal si operaciu: [init|destroy|produce|consume]\n");
    return 1;
  }

  if (argc < 3 ) {
    printf("Nezadal si prislusny suffix.\n");
    return 1;
  }

  srand(time(NULL));

  shared_names names;
  names.shm_name_ = add_suffix("HAD-SHM", argv[2]);
  names.mut_pc_ = add_suffix("MUT-PC", argv[2]);
  names.sem_produce_ = add_suffix("SEM-P", argv[2]);
  names.sem_consume_ = add_suffix("SEM-C", argv[2]);
  names.pipe1 = add_suffix("PIPE1", argv[2]);
  names.pipe2 = add_suffix("PIPE2", argv[2]);

  if (strcmp(argv[1], "init") == 0) {
    shm_init(&names);
    syn_shm_buffer_init(&names);
    pipe_init(names.pipe1);
    pipe_init(names.pipe2);
  } else if (strcmp(argv[1], "destroy") == 0) {
    shm_destroy(&names);
    syn_shm_buffer_destroy(&names);
    pipe_destroy(names.pipe1);
    pipe_destroy(names.pipe2);
  } else if (strcmp(argv[1], "produce") == 0) {
    dispatch_producer(&names, ' ', true);
  } else if (strcmp(argv[1], "consume") == 0) {
    run_consumer(&names);
  } 

  destroy_names(&names);
  return 0;
}
