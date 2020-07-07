#pragma once

#include "trico_api.h"

#include <stdint.h>

namespace trico
  {

  TRICO_API void transpose_aos_to_soa(float** x, float** y, float** z, const float* vertices, uint32_t nr_of_vertices);

  TRICO_API void transpose_soa_to_aos(float** vertices, const float* x, const float* y, const float* z, uint32_t nr_of_vertices);

  }