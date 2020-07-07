#pragma once

#include "trico_api.h"
#include <stdint.h>
#include <iostream>

namespace trico
  {
  TRICO_API void compress(uint8_t* out, const float* input, const uint64_t number_of_floats);
  }
