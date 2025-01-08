#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "syn_buffer.h"
#include "snake.h"
#include "../sockets-lib/socket.h"

#define SOCKET_PC 4921

void run_consumer(shared_names* names) {
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, names);
  
  int fd_passive = passive_socket_init(SOCKET_PC);
  int fd_active = passive_socket_wait_for_client(fd_passive);

  game_world world;
  //init_game_without_obs(10, &world);
  init_game_with_obs(10, 5, &world);

  for (size_t i = 0; i < world.size; i++) {
    write(fd_active, world.field[i], world.size * sizeof(char));
  }

  char output;
  syn_shm_buffer_pop(&syn, &output);

  bool isUserPlaying = true;
  while (output != 'k') {
    if (!move_snake(&output, &world)) {
      isUserPlaying = false;
      write(fd_active, &isUserPlaying, sizeof(bool));
      break;
    }
    write(fd_active, &isUserPlaying, sizeof(bool));
    
    for (size_t i = 0; i < world.size; i++) {
      write(fd_active, world.field[i], world.size * sizeof(char));
    }
    syn_shm_buffer_pop(&syn, &output);
  }
  
  active_socket_destroy(fd_active);
  passive_socket_destroy(fd_passive);

  syn_shm_buffer_close(&syn);

  destroy_game(&world);
}

typedef struct producer_data {
  shared_names* names;
} producer_data;

void * run_producer_output(void* arg) {
  int fd_active = connect_to_server("localhost", SOCKET_PC);
  game_world world;
  world.size = 10;
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
    }
  }
  printf("Hra ukončená.");

  active_socket_destroy(fd_active);
  return NULL;
}

char get_user_input() {
  char input;
  scanf(" %c", &input);
  if (input != 'k' && input != 'w' && input != 'a' && input != 's' && input != 'd') {
    printf("Nesprávny výber činnosti!\n");
    printf("Zadaj smer pohybu [w|a|s|d] | ukonči hru [k]\n");
    return get_user_input();
  } 
  return input;
}

void * run_producer_input(void* arg) {
  producer_data* data = arg;
  synchronized_buffer syn;
  syn_shm_buffer_open(&syn, data->names);

  char input = ' ';
  while (input != 'k') {
    input = get_user_input();
    syn_shm_buffer_push(&syn, &input);
  }

  syn_shm_buffer_close(&syn);
  return NULL;
}

void dispatch_producer(shared_names* names) {
  producer_data data = {names};
  pthread_t input;
  pthread_t output;
  pthread_create(&input, NULL, run_producer_input, &data);
  pthread_create(&output, NULL, run_producer_output, &data);
  pthread_join(input, NULL);
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

  if (strcmp(argv[1], "init") == 0) {
    shm_init(&names);
    syn_shm_buffer_init(&names);
  } else if (strcmp(argv[1], "destroy") == 0) {
    shm_destroy(&names);
    syn_shm_buffer_destroy(&names);
  } else if (strcmp(argv[1], "produce") == 0) {
    printf("Zadaj smer pohybu [w|a|s|d] | ukonči hru [k]\n");
    dispatch_producer(&names);
  } else if (strcmp(argv[1], "consume") == 0) {
    run_consumer(&names);
  }
  destroy_names(&names);
  return 0;
}
