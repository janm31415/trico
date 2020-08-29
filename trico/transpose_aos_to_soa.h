#pragma once

#include "trico_api.h"

#include <stdint.h>

namespace trico
  {

  TRICO_API void transpose_xyz_aos_to_soa(float** x, float** y, float** z, const float* vertices, uint32_t nr_of_vertices);

  TRICO_API void transpose_xyz_soa_to_aos(float** vertices, const float* x, const float* y, const float* z, uint32_t nr_of_vertices);

  TRICO_API void transpose_xyz_aos_to_soa(double** x, double** y, double** z, const double* vertices, uint32_t nr_of_vertices);

  TRICO_API void transpose_xyz_soa_to_aos(double** vertices, const double* x, const double* y, const double* z, uint32_t nr_of_vertices);

  TRICO_API void transpose_uint32_aos_to_soa(uint8_t** b1, uint8_t** b2, uint8_t** b3, uint8_t** b4, const uint32_t* indices, uint32_t nr_of_indices);

  TRICO_API void transpose_uint32_soa_to_aos(uint32_t** indices, const uint8_t* b1, const uint8_t* b2, const uint8_t* b3, const uint8_t* b4, uint32_t nr_of_indices);

  }