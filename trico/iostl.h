#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TRICO_IOSTL_H
#define TRICO_IOSTL_H

#include "trico_api.h"

#include <stdint.h>

/*
Returns 1 if no errors.
Memory of vertices and triangles should be cleaned up with free.
*/
TRICO_API int trico_read_stl(uint32_t* nr_of_vertices, float** vertices, uint32_t* nr_of_triangles, uint32_t** triangles, const char* filename);

TRICO_API int trico_read_stl_full(uint32_t* nr_of_vertices, float** vertices, uint32_t* nr_of_triangles, uint32_t** triangles, float** normals, uint16_t** attributes, const char* filename);

TRICO_API int trico_write_stl(const float* vertices, const uint32_t* triangles, const uint32_t nr_of_triangles, const float* triangle_normals, const uint16_t* attributes, const char* filename);

#endif

#if defined (__cplusplus)
  }
#endif