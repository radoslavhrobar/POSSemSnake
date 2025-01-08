#ifndef BUFFER
#define BUFFER

#include <stddef.h>

typedef struct buffer {
  char data;
  size_t capacity_;
  size_t size_;
} buffer;

void buffer_init(buffer* this);
void buffer_destroy(buffer* this);
void buffer_push(buffer* this, const char* input);
void buffer_pop(buffer* this, char* output);

#endif

