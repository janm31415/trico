#pragma once

#include <stdlib.h>
#include <stdint.h>

namespace trico
  {

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
  }