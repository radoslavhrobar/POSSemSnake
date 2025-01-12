#ifndef CLIENT
#define CLIENT

#include <stdatomic.h>
#include "names.h"

#define SOCKET_PC 2984

typedef struct producer_data {
  shared_names* names;
  char input;
  atomic_bool isUserPlaying;
  atomic_bool paused;
} producer_data;

void print_basic_info();
char get_first_input();
char get_user_input();
char get_user_input_pause();
char get_game_mode();
char get_game_type();
int get_dimension();
double get_game_time(); 
void * run_producer_output(void* arg);
void* run_producer_input(void* arg);
void* run_producer_input_push(void* arg); 
void dispatch_producer(shared_names* names, char in, atomic_bool isUserPlaying, atomic_bool paused);

#endif
