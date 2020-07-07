#pragma once

#include "trico_api.h"

#include <stdint.h>

namespace trico
  {
  /*
  Returns 0 if no errors.
  Memory of vertices and triangles should be cleaned up with free.
  */
  TRICO_API int read_stl(uint32_t* nr_of_vertices, float** vertices, uint32_t* nr_of_triangles, uint32_t** triangles, const char* filename);

  }