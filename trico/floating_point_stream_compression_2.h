#pragma once

#include "trico_api.h"
#include <stdint.h>
#include <iostream>

namespace trico
  {
  TRICO_API void compress_2(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats);

  TRICO_API void decompress_2(uint32_t* number_of_floats, float** out, const uint8_t* compressed);
  }
