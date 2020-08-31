#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_TRICO_H
#define TRICO_TRICO_H

#include "trico_api.h"
#include <stdint.h>

enum trico_stream_type
  {
  empty,
  vertex_float_stream,
  vertex_double_stream,
  triangle_uint32_stream,
  triangle_uint64_stream,
  uv_float_stream,
  uv_double_stream,
  normal_float_stream,
  normal_double_stream,
  attribute_float_stream,
  attribute_double_stream,
  attribute_uint8_stream,
  attribute_uint16_stream,
  attribute_uint32_stream,
  attribute_uint64_stream
  };

TRICO_API void* trico_open_archive_for_writing(uint64_t initial_buffer_size);
TRICO_API void* trico_open_archive_for_reading(const uint8_t* data, uint64_t data_size);
TRICO_API void trico_close_archive(void* archive);

TRICO_API int trico_write_vertices(void* archive, const float* vertices, uint32_t nr_of_vertices);
TRICO_API int trico_write_vertices_double(void* archive, const double* vertices, uint32_t nr_of_vertices);
TRICO_API int trico_write_triangles(void* archive, const uint32_t* tria_indices, uint32_t nr_of_triangles);
TRICO_API int trico_write_triangles_long(void* archive, const uint64_t* tria_indices, uint32_t nr_of_triangles);
TRICO_API int trico_write_uv(void* archive, const float* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_uv_double(void* archive, const double* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_normals(void* archive, const float* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_normals_double(void* archive, const double* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_attributes_float(void* archive, const float* attrib, uint32_t nr_of_attribs);
TRICO_API int trico_write_attributes_double(void* archive, const double* attrib, uint32_t nr_of_attribs);
TRICO_API int trico_write_attributes_uint8(void* archive, const uint8_t* attrib, uint32_t nr_of_attribs);
TRICO_API int trico_write_attributes_uint16(void* archive, const uint16_t* attrib, uint32_t nr_of_attribs);
TRICO_API int trico_write_attributes_uint32(void* archive, const uint32_t* attrib, uint32_t nr_of_attribs);
TRICO_API int trico_write_attributes_uint64(void* archive, const uint64_t* attrib, uint32_t nr_of_attribs);

TRICO_API uint8_t* trico_get_buffer_pointer(void* archive);
TRICO_API uint64_t trico_get_size(void* archive);

TRICO_API uint32_t trico_get_version(void* archive);
TRICO_API enum trico_stream_type trico_get_next_stream_type(void* archive);

TRICO_API uint32_t trico_get_number_of_vertices(void* archive);
TRICO_API uint32_t trico_get_number_of_triangles(void* archive);
TRICO_API uint32_t trico_get_number_of_uvs(void* archive);
TRICO_API uint32_t trico_get_number_of_normals(void* archive);
TRICO_API uint32_t trico_get_number_of_attributes(void* archive);

TRICO_API int trico_read_vertices(void* archive, float** vertices);
TRICO_API int trico_read_vertices_double(void* archive, double** vertices);
TRICO_API int trico_read_triangles(void* archive, uint32_t** triangles);
TRICO_API int trico_read_triangles_long(void* archive, uint64_t** triangles);
TRICO_API int trico_read_uv(void* archive, float** uv);
TRICO_API int trico_read_uv_double(void* archive, double** uv);
TRICO_API int trico_read_normals(void* archive, float** normals);
TRICO_API int trico_read_normals_double(void* archive, double** normals);
TRICO_API int trico_read_attributes_float(void* archive, float** attrib);
TRICO_API int trico_read_attributes_double(void* archive, double** attrib);
TRICO_API int trico_read_attributes_uint8(void* archive, uint8_t** attrib);
TRICO_API int trico_read_attributes_uint16(void* archive, uint16_t** attrib);
TRICO_API int trico_read_attributes_uint32(void* archive, uint32_t** attrib);
TRICO_API int trico_read_attributes_uint64(void* archive, uint64_t** attrib);
TRICO_API int trico_skip_next_stream(void* archive);

#endif

#if defined (__cplusplus)
  }
#endif