#include "buffer.h"

void buffer_init(buffer* this) {
  this->capacity_ = BUFFER_CAPACITY;
  this->size_ = 0;
}

void buffer_destroy(buffer* this) {
}

void buffer_push(buffer* this, const char* input) {
  this->data = *input;
  this->size_++;
}

void buffer_pop(buffer* this, char* output) {
  *output = this->data;
  this->size_--;
}
