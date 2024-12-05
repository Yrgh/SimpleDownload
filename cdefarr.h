#ifndef _DEFARR_H_
#define _DEFARR_H_
#include <stdlib.h>
#include <stdint.h>

void *reallocate(void *pointer, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }
  
  void *result = realloc(pointer, new_size);
  if (result == NULL) exit(1);
  return result;
}



#define DEFINE_ARRAY_TYPE(type, name) \
typedef struct {           \
  uint32_t size, capacity; \
  type *data;              \
} _##name##_base;          \
typedef _##name##_base* name; \
void name##_init(name arr) {  \
  arr->size = 0;              \
  arr->capacity = 0;          \
  arr->data = NULL;           \
}                             \
void name##_write(name arr, type *data) {  \
  if (arr->capacity < arr->size + 1) {    \
    arr->capacity = arr->capacity < 8 ?   \
      8 : arr->capacity * 2;              \
    arr->data = (type *)reallocate(       \
      arr->data,                          \
      arr->capacity * sizeof(type)        \
    );                                    \
  }                                       \
  arr->data[arr->size] = *data;           \
  arr->size++;                            \
}                                         \
type *name##_read(name arr, uint32_t index) {   \
  if (index >= arr->size || index < 0) exit(1); \
  return arr->data + index;                     \
}                                               \
void name##_push_back(name arr, type data) { \
  name##_write(arr, &data);                  \
}                                            \
void name##_pop_back(name arr) { \
  if (arr->size == 0) exit(1);   \
  arr->size--;                   \
}                                \
type name##_get(name arr, uint32_t index) { \
  return *name##_read(arr, index);          \
}                                           \
void name##_destroy(name arr) {  \
  reallocate(arr->data, 0);      \
  name##_init(arr);              \
}                                \

#endif
