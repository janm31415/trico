#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_TRICO_H
#define TRICO_TRICO_H

#include "trico_api.h"
#include <stdint.h>

enum e_stream_type
  {
  empty,
  vertex_float_stream,
  vertex_double_stream,
  triangle_uint32_stream,
  triangle_uint64_stream
  };

TRICO_API void* trico_open_archive_for_writing(uint64_t initial_buffer_size);
TRICO_API void* trico_open_archive_for_reading(const uint8_t* data, uint64_t data_size);
TRICO_API void trico_close_archive(void* archive);

TRICO_API int trico_write_vertices(void* archive, uint32_t nr_of_vertices, const float* vertices);
TRICO_API int trico_write_vertices_double(void* archive, uint32_t nr_of_vertices, const double* vertices);
TRICO_API int trico_write_triangles(void* archive, uint32_t nr_of_triangles, const uint32_t* tria_indices);
TRICO_API int trico_write_triangles_long(void* archive, uint32_t nr_of_triangles, const uint64_t* tria_indices);

TRICO_API uint8_t* trico_get_buffer_pointer(void* archive);
TRICO_API uint64_t trico_get_size(void* archive);

TRICO_API uint32_t trico_get_version(void* archive);
TRICO_API enum e_stream_type trico_get_next_stream_type(void* archive);

TRICO_API uint32_t trico_get_number_of_vertices(void* archive);
TRICO_API uint32_t trico_get_number_of_triangles(void* archive);

TRICO_API int trico_read_vertices(void* archive, float** vertices);
TRICO_API int trico_read_vertices_double(void* archive, double** vertices);
TRICO_API int trico_read_triangles(void* archive, uint32_t** triangles);
TRICO_API int trico_read_triangles_long(void* archive, uint64_t** triangles);

#endif

#if defined (__cplusplus)
  }
#endif