#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/transpose_aos_to_soa.h>

namespace trico
  {
  void transpose_aos_to_soa_standford_bunny()
    {
    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "data/StanfordBunny.stl"));

    float* x;
    float* y;
    float* z;

    transpose_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    for (uint32_t i = 0; i < nr_of_vertices; ++i)
      {
      TEST_EQ(x[i], vertices[i * 3]);
      TEST_EQ(y[i], vertices[i * 3 + 1]);
      TEST_EQ(z[i], vertices[i * 3 + 2]);
      }

    float* vertices_from_xyz;

    transpose_soa_to_aos(&vertices_from_xyz, x, y, z, nr_of_vertices);

    for (uint32_t i = 0; i < nr_of_vertices; ++i)
      {
      TEST_EQ(x[i], vertices_from_xyz[i * 3]);
      TEST_EQ(y[i], vertices_from_xyz[i * 3 + 1]);
      TEST_EQ(z[i], vertices_from_xyz[i * 3 + 2]);
      }

    trico_free(vertices_from_xyz);
    trico_free(x);
    trico_free(y);
    trico_free(z);
    trico_free(vertices);
    trico_free(triangles);
    }
  }

void run_all_fps_compression_tests()
  {
  using namespace trico;
  transpose_aos_to_soa_standford_bunny();
  }