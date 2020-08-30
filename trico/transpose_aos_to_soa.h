#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_TRANSPOSE_AOS_TO_SOA_H
#define TRICO_TRANSPOSE_AOS_TO_SOA_H

#include "trico_api.h"

#include <stdint.h>

TRICO_API void trico_transpose_xyz_aos_to_soa(float** x, float** y, float** z, const float* vertices, uint32_t nr_of_vertices);

TRICO_API void trico_transpose_xyz_soa_to_aos(float** vertices, const float* x, const float* y, const float* z, uint32_t nr_of_vertices);

TRICO_API void trico_transpose_xyz_aos_to_soa_double_precision(double** x, double** y, double** z, const double* vertices, uint32_t nr_of_vertices);

TRICO_API void trico_transpose_xyz_soa_to_aos_double_precision(double** vertices, const double* x, const double* y, const double* z, uint32_t nr_of_vertices);

TRICO_API void trico_transpose_uv_aos_to_soa(float** u, float** v, const float* uv, uint32_t nr_of_uv_positions);

TRICO_API void trico_transpose_uv_soa_to_aos(float** uv, const float* u, const float* v, uint32_t nr_of_uv_positions);

TRICO_API void trico_transpose_uv_aos_to_soa_double_precision(double** u, double** v, const double* uv, uint32_t nr_of_uv_positions);

TRICO_API void trico_transpose_uv_soa_to_aos_double_precision(double** uv, const double* u, const double* v, uint32_t nr_of_uv_positions);

TRICO_API void trico_transpose_uint16_aos_to_soa(uint8_t** b1, uint8_t** b2, const uint16_t* indices, uint32_t nr_of_indices);

TRICO_API void trico_transpose_uint16_soa_to_aos(uint16_t** indices, const uint8_t* b1, const uint8_t* b2, uint32_t nr_of_indices);

TRICO_API void trico_transpose_uint32_aos_to_soa(uint8_t** b1, uint8_t** b2, uint8_t** b3, uint8_t** b4, const uint32_t* indices, uint32_t nr_of_indices);

TRICO_API void trico_transpose_uint32_soa_to_aos(uint32_t** indices, const uint8_t* b1, const uint8_t* b2, const uint8_t* b3, const uint8_t* b4, uint32_t nr_of_indices);

TRICO_API void trico_transpose_uint64_aos_to_soa(uint8_t** b1, uint8_t** b2, uint8_t** b3, uint8_t** b4, uint8_t** b5, uint8_t** b6, uint8_t** b7, uint8_t** b8, const uint64_t* indices, uint32_t nr_of_indices);

TRICO_API void trico_transpose_uint64_soa_to_aos(uint64_t** indices, const uint8_t* b1, const uint8_t* b2, const uint8_t* b3, const uint8_t* b4, const uint8_t* b5, const uint8_t* b6, const uint8_t* b7, const uint8_t* b8, uint32_t nr_of_indices);

#endif

#if defined (__cplusplus)
  }
#endif