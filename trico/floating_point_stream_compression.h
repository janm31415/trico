#pragma once

#include "trico_api.h"
#include <stdint.h>

namespace trico
  {
  TRICO_API void compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats, uint32_t hash1_size_exponent = 4, uint32_t hash2_size_exponent = 10);

  TRICO_API void decompress(uint32_t* number_of_floats, float** out, const uint8_t* compressed);



  TRICO_API void compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const double* input, const uint32_t number_of_doubles, uint64_t hash1_size_exponent = 20, uint64_t hash2_size_exponent = 20);

  TRICO_API void decompress(uint32_t* number_of_doubles, double** out, const uint8_t* compressed);
  }
