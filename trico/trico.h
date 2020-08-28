#pragma once

#include "trico_api.h"
#include <stdint.h>

namespace trico
  {

  TRICO_API void* open_writable_archive(const char* filename);

  TRICO_API void* open_readable_archive(const char* filename);

  TRICO_API void close_archive(void* archive);

  TRICO_API void write_vertices(void* archive, const float* vertices, uint32_t nr_of_vertices);
  
  TRICO_API void write_triangles(void* archive, const uint32_t* tria_indices, uint32_t nr_of_triangles);

  TRICO_API uint32_t get_version(void* archive);

  } // namespace trico