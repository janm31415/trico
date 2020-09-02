#if defined (__cplusplus)
extern "C" {
#endif // #if defined (__cplusplus)

#ifndef TRICO_IOPLY_H
#define TRICO_IOPLY_H

#include "trico_api.h"

#include <stdint.h>

TRICO_API int trico_read_ply(uint32_t* nr_of_vertices, float** vertices, float** vertex_normals, uint32_t** vertex_colors, uint32_t* nr_of_triangles, uint32_t** triangles, float** texcoords, const char* filename);

TRICO_API int trico_write_ply(const uint32_t nr_of_vertices, const float* vertices, const float* vertex_normals, const uint32_t* vertex_colors, const uint32_t nr_of_triangles, const uint32_t* triangles, const float* texcoords, const char* filename);

#endif // #ifndef TRICO_IOPLY_H

#if defined (__cplusplus)
  }
#endif // #if defined (__cplusplus)