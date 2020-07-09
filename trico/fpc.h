#pragma once

#include "trico_api.h"
#include <stdint.h>
#include <iostream>

namespace trico
  {
  TRICO_API void fpc_compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const double* input, const uint32_t number_of_doubles);

  TRICO_API void fpc_decompress(uint32_t* number_of_doubles, double** out, const uint8_t* compressed);
  }