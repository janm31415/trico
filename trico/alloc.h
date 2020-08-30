#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_ALLOC_H
#define TRICO_ALLOC_H

#include <stdlib.h>
#include <stdint.h>


inline void* trico_malloc(size_t size)
  {
  return malloc(size);
  }

inline void* trico_realloc(void* ptr, size_t new_size)
  {
  return realloc(ptr, new_size);
  }

inline void trico_free(void* ptr)
  {
  free(ptr);
  }

#endif

#if defined (__cplusplus)
  }
#endif