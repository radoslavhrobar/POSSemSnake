#include <stdlib.h>
#include <string.h>

#include "names.h"

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
}

