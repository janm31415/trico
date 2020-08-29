#pragma once

#include "trico_api.h"
#include <stdint.h>

namespace trico
  {

  enum e_stream_type
    {
    empty,
    vertex_float_stream,
    vertex_double_stream,
    triangle_uint32_stream,
    triangle_uint64_stream
    };

  TRICO_API void* open_old_archive_for_writing(const char* filename);

  TRICO_API void* open_old_archive_for_reading(const char* filename);

  TRICO_API void close_old_archive(void* archive);

  TRICO_API void write_vertices_old(void* archive, uint32_t nr_of_vertices, const float* vertices);

  TRICO_API void write_vertices_old(void* archive, uint32_t nr_of_vertices, const double* vertices);
  
  TRICO_API void write_triangles_old(void* archive, uint32_t nr_of_triangles, const uint32_t* tria_indices);  

  TRICO_API void write_triangles_old(void* archive, uint32_t nr_of_triangles, const uint64_t* tria_indices);

  TRICO_API uint32_t get_version_old(void* archive);

  TRICO_API e_stream_type get_next_stream_type_old(void* archive);

  TRICO_API bool read_vertices_old(void* archive, uint32_t* nr_of_vertices, float** vertices);

  TRICO_API bool read_vertices_old(void* archive, uint32_t* nr_of_vertices, double** vertices);

  TRICO_API bool read_triangles_old(void* archive, uint32_t* nr_of_triangles, uint32_t** tria_indices);

  TRICO_API bool read_triangles_old(void* archive, uint32_t* nr_of_triangles, uint64_t** tria_indices);




  TRICO_API void* open_archive_for_writing(uint64_t initial_buffer_size);
  TRICO_API void* open_archive_for_reading(const uint8_t* data, uint64_t data_size);
  TRICO_API void close_archive(void* archive);

  TRICO_API void write_vertices(void* archive, uint32_t nr_of_vertices, const float* vertices);
  TRICO_API void write_vertices(void* archive, uint32_t nr_of_vertices, const double* vertices);
  TRICO_API void write_triangles(void* archive, uint32_t nr_of_triangles, const uint32_t* tria_indices);
  TRICO_API void write_triangles(void* archive, uint32_t nr_of_triangles, const uint64_t* tria_indices);

  TRICO_API uint8_t* get_buffer_pointer(void* archive);
  TRICO_API uint64_t get_size(void* archive);

  TRICO_API uint32_t get_version(void* archive);
  TRICO_API e_stream_type get_next_stream_type(void* archive);

  TRICO_API uint32_t get_number_of_vertices(void* archive);
  TRICO_API uint32_t get_number_of_triangles(void* archive);

  TRICO_API bool read_vertices(void* archive, float** vertices);
  TRICO_API bool read_vertices(void* archive, double** vertices);
  TRICO_API bool read_triangles(void* archive, uint32_t** triangles);
  TRICO_API bool read_triangles(void* archive, uint64_t** triangles);

  } // namespace trico