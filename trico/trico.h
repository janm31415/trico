#if defined (__cplusplus)
extern "C" {
#endif // #if defined (__cplusplus)

#ifndef TRICO_TRICO_H
#define TRICO_TRICO_H

#include "trico_api.h"
#include <stdint.h>

enum trico_stream_type
  {
  trico_empty,
  trico_vertex_float_stream,
  trico_vertex_double_stream,
  trico_triangle_uint32_stream,
  trico_triangle_uint64_stream,
  trico_uv_per_vertex_float_stream,
  trico_uv_per_vertex_double_stream,
  trico_uv_per_triangle_float_stream,
  trico_uv_per_triangle_double_stream,
  trico_vertex_normal_float_stream,
  trico_vertex_normal_double_stream,
  trico_triangle_normal_float_stream,
  trico_triangle_normal_double_stream,
  trico_vertex_color_stream,
  trico_triangle_color_stream,
  trico_attribute_float_stream,
  trico_attribute_double_stream,
  trico_attribute_uint8_stream,
  trico_attribute_uint16_stream,
  trico_attribute_uint32_stream,
  trico_attribute_uint64_stream
  };

TRICO_API void* trico_open_archive_for_writing(uint64_t initial_buffer_size);
TRICO_API void* trico_open_archive_for_reading(const uint8_t* data, uint64_t data_size);
TRICO_API void trico_close_archive(void* archive);

TRICO_API int trico_write_vertices(void* archive, const float* vertices, uint32_t nr_of_vertices);
TRICO_API int trico_write_vertices_double(void* archive, const double* vertices, uint32_t nr_of_vertices);
TRICO_API int trico_write_triangles(void* archive, const uint32_t* tria_indices, uint32_t nr_of_triangles);
TRICO_API int trico_write_triangles_long(void* archive, const uint64_t* tria_indices, uint32_t nr_of_triangles);
TRICO_API int trico_write_uv_per_vertex(void* archive, const float* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_uv_per_vertex_double(void* archive, const double* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_uv_per_triangle(void* archive, const float* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_uv_per_triangle_double(void* archive, const double* uv, uint32_t nr_of_uv_positions);
TRICO_API int trico_write_vertex_normals(void* archive, const float* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_vertex_normals_double(void* archive, const double* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_triangle_normals(void* archive, const float* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_triangle_normals_double(void* archive, const double* normals, uint32_t nr_of_normals);
TRICO_API int trico_write_vertex_colors(void* archive, const uint32_t* color, uint32_t nr_of_colors);
TRICO_API int trico_write_triangle_colors(void* archive, const uint32_t* color, uint32_t nr_of_colors);
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
TRICO_API uint32_t trico_get_number_of_colors(void* archive);
TRICO_API uint32_t trico_get_number_of_attributes(void* archive);

TRICO_API int trico_read_vertices(void* archive, float** vertices);
TRICO_API int trico_read_vertices_double(void* archive, double** vertices);
TRICO_API int trico_read_triangles(void* archive, uint32_t** triangles);
TRICO_API int trico_read_triangles_long(void* archive, uint64_t** triangles);
TRICO_API int trico_read_uv_per_vertex(void* archive, float** uv);
TRICO_API int trico_read_uv_per_vertex_double(void* archive, double** uv);
TRICO_API int trico_read_uv_per_triangle(void* archive, float** uv);
TRICO_API int trico_read_uv_per_triangle_double(void* archive, double** uv);
TRICO_API int trico_read_vertex_normals(void* archive, float** normals);
TRICO_API int trico_read_vertex_normals_double(void* archive, double** normals);
TRICO_API int trico_read_triangle_normals(void* archive, float** normals);
TRICO_API int trico_read_triangle_normals_double(void* archive, double** normals);
TRICO_API int trico_read_vertex_colors(void* archive, uint32_t** color);
TRICO_API int trico_read_triangle_colors(void* archive, uint32_t** color);
TRICO_API int trico_read_attributes_float(void* archive, float** attrib);
TRICO_API int trico_read_attributes_double(void* archive, double** attrib);
TRICO_API int trico_read_attributes_uint8(void* archive, uint8_t** attrib);
TRICO_API int trico_read_attributes_uint16(void* archive, uint16_t** attrib);
TRICO_API int trico_read_attributes_uint32(void* archive, uint32_t** attrib);
TRICO_API int trico_read_attributes_uint64(void* archive, uint64_t** attrib);
TRICO_API int trico_skip_next_stream(void* archive);

#endif // #ifndef TRICO_TRICO_H

#if defined (__cplusplus)
  }
#endif // #if defined (__cplusplus)