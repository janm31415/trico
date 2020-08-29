#include "transpose_aos_to_soa.h"
#include "alloc.h"

#include "intrinsics.h"

namespace trico
  {

  //https://software.intel.com/content/www/us/en/develop/articles/3d-vector-normalization-using-256-bit-intel-advanced-vector-extensions-intel-avx.html

  void transpose_xyz_aos_to_soa(float** x, float** y, float** z, const float* vertices, uint32_t nr_of_vertices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*x)[i] = *vertices++;
      (*y)[i] = *vertices++;
      (*z)[i] = *vertices++;
      }
    }


  void transpose_xyz_soa_to_aos(float** vertices, const float* x, const float* y, const float* z, uint32_t nr_of_vertices)
    {    
    uint32_t treated = 0;
    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*vertices)[i * 3] = *x++;
      (*vertices)[i * 3 + 1] = *y++;
      (*vertices)[i * 3 + 2] = *z++;
      }
    }

  void transpose_xyz_aos_to_soa(double** x, double** y, double** z, const double* vertices, uint32_t nr_of_vertices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*x)[i] = *vertices++;
      (*y)[i] = *vertices++;
      (*z)[i] = *vertices++;
      }
    }


  void transpose_xyz_soa_to_aos(double** vertices, const double* x, const double* y, const double* z, uint32_t nr_of_vertices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*vertices)[i * 3] = *x++;
      (*vertices)[i * 3 + 1] = *y++;
      (*vertices)[i * 3 + 2] = *z++;
      }
    }

  void transpose_uint32_aos_to_soa(uint8_t** b1, uint8_t** b2, uint8_t** b3, uint8_t** b4, const uint32_t* indices, uint32_t nr_of_indices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_indices; ++i)
      {
      uint32_t index = *indices++;
      (*b1)[i] = index & 0xff;
      (*b2)[i] = (index >> 8) & 0xff;
      (*b3)[i] = (index >> 16) & 0xff;
      (*b4)[i] = (index >> 24) & 0xff;
      }
    }

  void transpose_uint32_soa_to_aos(uint32_t** indices, const uint8_t* b1, const uint8_t* b2, const uint8_t* b3, const uint8_t* b4, uint32_t nr_of_indices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_indices; ++i)
      {
      const uint32_t index = (uint32_t)(*b1++) | (uint32_t)(*b2++) << 8 | (uint32_t)(*b3++) << 16 | (uint32_t)(*b4++) << 24;
      (*indices)[i] = index;
      }
    }

  void transpose_uint64_aos_to_soa(uint8_t** b1, uint8_t** b2, uint8_t** b3, uint8_t** b4, uint8_t** b5, uint8_t** b6, uint8_t** b7, uint8_t** b8, const uint64_t* indices, uint32_t nr_of_indices)
    {
    uint32_t treated = 0;

    for (uint32_t i = treated; i < nr_of_indices; ++i)
      {
      uint64_t index = *indices++;
      (*b1)[i] = index & 0xff;
      (*b2)[i] = (index >> 8) & 0xff;
      (*b3)[i] = (index >> 16) & 0xff;
      (*b4)[i] = (index >> 24) & 0xff;
      (*b5)[i] = (index >> 32) & 0xff;
      (*b6)[i] = (index >> 40) & 0xff;
      (*b7)[i] = (index >> 48) & 0xff;
      (*b8)[i] = (index >> 56) & 0xff;
      }
    }

  void transpose_uint64_soa_to_aos(uint64_t** indices, const uint8_t* b1, const uint8_t* b2, const uint8_t* b3, const uint8_t* b4, const uint8_t* b5, const uint8_t* b6, const uint8_t* b7, const uint8_t* b8, uint32_t nr_of_indices)
    {
    uint32_t treated = 0;
    for (uint32_t i = treated; i < nr_of_indices; ++i)
      {
      const uint64_t index = (uint64_t)(*b1++) | (uint64_t)(*b2++) << 8 | (uint64_t)(*b3++) << 16 | (uint64_t)(*b4++) << 24 | (uint64_t)(*b5++) << 32 | (uint64_t)(*b6++) << 40 | (uint64_t)(*b7++) << 48 | (uint64_t)(*b8++) << 56;
      (*indices)[i] = index;
      }
    }

  }