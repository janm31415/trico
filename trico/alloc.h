#pragma once

#include "intrinsics.h"

namespace trico
  {

  inline void* trico_malloc(size_t size)
    {
#if defined(_AVX2)
#ifdef _WIN32
    return _aligned_malloc(size, 32);
#else
    return aligned_alloc(32, size);
#endif
#elif defined(_SSE2)
#ifdef _WIN32
    return _aligned_malloc(size, 16);
#else
    return aligned_alloc(16, size);
#endif
#else
    return malloc(size);
#endif
    }

  inline void* trico_realloc(void* ptr, size_t new_size)
    {
#if defined(_AVX2)
#ifdef _WIN32
    return _aligned_realloc(ptr, new_size, 32);
#else
    void* new_mem = aligned_alloc(32, new_size);
    size_t old_size = malloc_usable_size(ptr);
    memcpy(new_mem, ptr, old_size > new_size ? new_size : old_size);
    free(ptr);
    return new_mem;
#endif
#elif defined(_SSE2)
#ifdef _WIN32
    return _aligned_realloc(ptr, new_size, 16);
#else
    void* new_mem = aligned_alloc(16, new_size);
    size_t old_size = malloc_usable_size(ptr);
    memcpy(new_mem, ptr, old_size > new_size ? new_size : old_size);
    free(ptr);
    return new_mem;
#endif
#else
    return realloc(ptr, new_size);
#endif
    }

  inline void trico_free(void* ptr)
    {
#if defined(_AVX2)
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
#elif defined(_SSE2)
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
#else
    free(ptr);
#endif
    }
  }