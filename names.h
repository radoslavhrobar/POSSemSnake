#ifndef NAMES
#define NAMES

typedef struct shared_names {
  char* shm_name_;
  char *mut_pc_;
  char *sem_produce_;
  char *sem_consume_;
  char *pipe1;
} shared_names;

void destroy_names(shared_names* names);

#endif


