#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_FLOATING_POINT_STREAM_COMPRESSION_H
#define TRICO_FLOATING_POINT_STREAM_COMPRESSION_H

#include "trico_api.h"
#include <stdint.h>

TRICO_API void trico_compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats, uint32_t hash1_size_exponent, uint32_t hash2_size_exponent); // 4, 10

TRICO_API void trico_decompress(uint32_t* number_of_floats, float** out, const uint8_t* compressed);

TRICO_API void trico_compress_double_precision(uint32_t* nr_of_compressed_bytes, uint8_t** out, const double* input, const uint32_t number_of_doubles, uint64_t hash1_size_exponent, uint64_t hash2_size_exponent); // 20, 20

TRICO_API void trico_decompress_double_precision(uint32_t* number_of_doubles, double** out, const uint8_t* compressed);

#endif

#if defined (__cplusplus)
  }
#endif