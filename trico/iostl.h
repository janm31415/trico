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

#endif

#if defined (__cplusplus)
  }
#endif