#include "transpose_aos_to_soa.h"
#include "alloc.h"

#include "intrinsics.h"

namespace trico
  {

  //https://software.intel.com/content/www/us/en/develop/articles/3d-vector-normalization-using-256-bit-intel-advanced-vector-extensions-intel-avx.html

  void transpose_aos_to_soa(float** x, float** y, float** z, const float* vertices, uint32_t nr_of_vertices)
    {
    *x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    *y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    *z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    uint32_t treated = 0;

#if defined(_AVX2)
    float* px = *x;
    float* py = *y;
    float* pz = *z;
    for (; treated < (nr_of_vertices & 7); treated += 8, vertices += 24, px += 8, py += 8, pz += 8)
      {
      __m128 *m = (__m128*) vertices;
      __m256 m03;
      __m256 m14;
      __m256 m25;
      m03 = _mm256_castps128_ps256(m[0]); // load lower halves
      m14 = _mm256_castps128_ps256(m[1]);
      m25 = _mm256_castps128_ps256(m[2]);
      m03 = _mm256_insertf128_ps(m03, m[3], 1);  // load upper halves
      m14 = _mm256_insertf128_ps(m14, m[4], 1);
      m25 = _mm256_insertf128_ps(m25, m[5], 1);

      __m256 xy = _mm256_shuffle_ps(m14, m25, _MM_SHUFFLE(2, 1, 3, 2)); // upper x's and y's 
      __m256 yz = _mm256_shuffle_ps(m03, m14, _MM_SHUFFLE(1, 0, 2, 1)); // lower y's and z's
      __m256 X = _mm256_shuffle_ps(m03, xy, _MM_SHUFFLE(2, 0, 3, 0));
      __m256 Y = _mm256_shuffle_ps(yz, xy, _MM_SHUFFLE(3, 1, 2, 0));
      __m256 Z = _mm256_shuffle_ps(yz, m25, _MM_SHUFFLE(3, 0, 3, 1));
      _mm256_store_ps(px, X);
      _mm256_store_ps(py, Y);
      _mm256_store_ps(pz, Z);
      }
#elif defined(_SSE2)
    float* px = *x;
    float* py = *y;
    float* pz = *z;
    for (; treated < (nr_of_vertices & 3); treated += 4, vertices += 12, px += 4, py += 4, pz += 4)
      {
      __m128 x0y0z0x1 = _mm_load_ps(vertices + 0);
      __m128 y1z1x2y2 = _mm_load_ps(vertices + 4);
      __m128 z2x3y3z3 = _mm_load_ps(vertices + 8);
      __m128 x2y2x3y3 = _mm_shuffle_ps(y1z1x2y2, z2x3y3z3, _MM_SHUFFLE(2, 1, 3, 2));
      __m128 y0z0y1z1 = _mm_shuffle_ps(x0y0z0x1, y1z1x2y2, _MM_SHUFFLE(1, 0, 2, 1));
      __m128 X = _mm_shuffle_ps(x0y0z0x1, x2y2x3y3, _MM_SHUFFLE(2, 0, 3, 0)); // x0x1x2x3
      __m128 Y = _mm_shuffle_ps(y0z0y1z1, x2y2x3y3, _MM_SHUFFLE(3, 1, 2, 0)); // y0y1y2y3
      __m128 Z = _mm_shuffle_ps(y0z0y1z1, z2x3y3z3, _MM_SHUFFLE(3, 0, 3, 1)); // z0z1z2z3
      _mm_store_ps(px, X);
      _mm_store_ps(py, Y);
      _mm_store_ps(pz, Z);
      }
#endif

    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*x)[i] = *vertices++;
      (*y)[i] = *vertices++;
      (*z)[i] = *vertices++;
      }
    }


  void transpose_soa_to_aos(float** vertices, const float* x, const float* y, const float* z, uint32_t nr_of_vertices)
    {
    *vertices = (float*)trico_malloc(sizeof(float)*nr_of_vertices * 3);
    uint32_t treated = 0;
#if defined(_AVX2)
    float* vert = *vertices;
    for (; treated < (nr_of_vertices & 7); treated += 8, vert += 24, x += 8, y += 8, z += 8)
      {
      __m256 X = _mm256_load_ps(x);
      __m256 Y = _mm256_load_ps(y);
      __m256 Z = _mm256_load_ps(z);
      __m128 *m = (__m128*) vert;

      __m256 rxy = _mm256_shuffle_ps(X, Y, _MM_SHUFFLE(2, 0, 2, 0));
      __m256 ryz = _mm256_shuffle_ps(Y, Z, _MM_SHUFFLE(3, 1, 3, 1));
      __m256 rzx = _mm256_shuffle_ps(Z, X, _MM_SHUFFLE(3, 1, 2, 0));

      __m256 r03 = _mm256_shuffle_ps(rxy, rzx, _MM_SHUFFLE(2, 0, 2, 0));
      __m256 r14 = _mm256_shuffle_ps(ryz, rxy, _MM_SHUFFLE(3, 1, 2, 0));
      __m256 r25 = _mm256_shuffle_ps(rzx, ryz, _MM_SHUFFLE(3, 1, 3, 1));

      m[0] = _mm256_castps256_ps128(r03);
      m[1] = _mm256_castps256_ps128(r14);
      m[2] = _mm256_castps256_ps128(r25);
      m[3] = _mm256_extractf128_ps(r03, 1);
      m[4] = _mm256_extractf128_ps(r14, 1);
      m[5] = _mm256_extractf128_ps(r25, 1);
      }
#elif defined(_SSE2)
    float* vert = *vertices;
    for (; treated < (nr_of_vertices & 3); treated += 4, vert += 12, x += 4, y += 4, z += 4)
      {
      __m128 X = _mm_load_ps(x);
      __m128 Y = _mm_load_ps(y);
      __m128 Z = _mm_load_ps(z);
      __m128 x0x2y0y2 = _mm_shuffle_ps(X, Y, _MM_SHUFFLE(2, 0, 2, 0));
      __m128 y1y3z1z3 = _mm_shuffle_ps(Y, Z, _MM_SHUFFLE(3, 1, 3, 1));
      __m128 z0z2x1x3 = _mm_shuffle_ps(Z, X, _MM_SHUFFLE(3, 1, 2, 0));

      __m128 rx0y0z0x1 = _mm_shuffle_ps(x0x2y0y2, z0z2x1x3, _MM_SHUFFLE(2, 0, 2, 0));
      __m128 ry1z1x2y2 = _mm_shuffle_ps(y1y3z1z3, x0x2y0y2, _MM_SHUFFLE(3, 1, 2, 0));
      __m128 rz2x3y3z3 = _mm_shuffle_ps(z0z2x1x3, y1y3z1z3, _MM_SHUFFLE(3, 1, 3, 1));

      _mm_store_ps(vert + 0, rx0y0z0x1);
      _mm_store_ps(vert + 4, ry1z1x2y2);
      _mm_store_ps(vert + 8, rz2x3y3z3);
      }
#endif
    for (uint32_t i = treated; i < nr_of_vertices; ++i)
      {
      (*vertices)[i * 3] = *x++;
      (*vertices)[i * 3 + 1] = *y++;
      (*vertices)[i * 3 + 2] = *z++;
      }
    }

  }