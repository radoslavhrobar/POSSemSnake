#ifndef SERVER
#define SERVER

#include <time.h>
#include "names.h"

#define SOCKET_PC 4921

time_t get_current_time();
void run_consumer(shared_names* names);

#endif
