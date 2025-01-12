#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <unistd.h>

#include "pipe.h"
#include "syn_buffer.h"
#include "client.h"
#include "server.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Nezadal si prislusny suffix.\n");
  }

  srand(time(NULL));

  shared_names names;
  names.shm_name_ = add_suffix("HAD-SHM", argv[1]);
  names.mut_pc_ = add_suffix("MUT-PC", argv[1]);
  names.sem_produce_ = add_suffix("SEM-P", argv[1]);
  names.sem_consume_ = add_suffix("SEM-C", argv[1]);
  names.pipe1 = add_suffix("PIPE1", argv[1]);
  
  if (argc == 3) {
    shm_destroy(&names);
    syn_shm_buffer_destroy(&names);
    pipe_destroy(names.pipe1);
    return 0;
  }

  shm_init(&names);
  syn_shm_buffer_init(&names);
  pipe_init(names.pipe1);

  pid_t pid = fork();

  if (pid < 0) {
    perror("Chyba volania fork!");
    return 2;
  }

  if (pid == 0) {
    run_consumer(&names);
  } else {
    dispatch_producer(&names, ' ', true, false);
  }

  destroy_names(&names);
  return 0;
}
