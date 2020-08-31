#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/transpose_aos_to_soa.h>
#include <trico/floating_point_stream_compression.h>

#include <lz4/lz4.h>

#include <iostream>

#include "timer.h"


namespace
  {

  timer g_timer;

  void tic()
    {
    g_timer.start();
    }

  void toc(const char* txt)
    {
    double ti = g_timer.time_elapsed();
    std::cout << txt << ti << " seconds.\n";
    }
  }

void transpose_xyz_aos_to_soa(const char* filename)
  {
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

  tic();
  trico_transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);
  toc("transpose_xyz_aos_to_soa time: ");

  for (uint32_t i = 0; i < nr_of_vertices; ++i)
    {
    TEST_EQ(x[i], vertices[i * 3]);
    TEST_EQ(y[i], vertices[i * 3 + 1]);
    TEST_EQ(z[i], vertices[i * 3 + 2]);
    }

  float* vertices_from_xyz = (float*)trico_malloc(sizeof(float)*nr_of_vertices * 3);

  tic();
  trico_transpose_xyz_soa_to_aos(&vertices_from_xyz, x, y, z, nr_of_vertices);
  toc("transpose_xyz_soa_to_aos time: ");

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

void compress_vertices_double(const char* filename)
  {
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

  trico_transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

  double* dx = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
  double* dy = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
  double* dz = (double*)trico_malloc(sizeof(double)*nr_of_vertices);

  for (uint32_t i = 0; i < nr_of_vertices; ++i)
    {
    dx[i] = (double)x[i];
    dy[i] = (double)y[i];
    dz[i] = (double)z[i];
    }

  uint32_t nr_of_compressed_x_bytes;
  uint8_t* compressed_x;
  trico_compress_double_precision(&nr_of_compressed_x_bytes, &compressed_x, dx, nr_of_vertices, 20, 20);

  uint32_t nr_of_doubles;
  double* decompressed_x;
  trico_decompress_double_precision(&nr_of_doubles, &decompressed_x, compressed_x);

  TEST_EQ(nr_of_vertices, nr_of_doubles);

  for (uint32_t i = 0; i < nr_of_doubles; ++i)
    TEST_EQ(dx[i], decompressed_x[i]);

  uint32_t nr_of_compressed_y_bytes;
  uint8_t* compressed_y;
  trico_compress_double_precision(&nr_of_compressed_y_bytes, &compressed_y, dy, nr_of_vertices, 20, 20);

  double* decompressed_y;
  trico_decompress_double_precision(&nr_of_doubles, &decompressed_y, compressed_y);

  TEST_EQ(nr_of_vertices, nr_of_doubles);

  for (uint32_t i = 0; i < nr_of_doubles; ++i)
    TEST_EQ(dy[i], decompressed_y[i]);

  uint32_t nr_of_compressed_z_bytes;
  uint8_t* compressed_z;
  trico_compress_double_precision(&nr_of_compressed_z_bytes, &compressed_z, dz, nr_of_vertices, 20, 20);

  double* decompressed_z;
  trico_decompress_double_precision(&nr_of_doubles, &decompressed_z, compressed_z);

  TEST_EQ(nr_of_vertices, nr_of_doubles);

  for (uint32_t i = 0; i < nr_of_doubles; ++i)
    TEST_EQ(dz[i], decompressed_z[i]);

  std::cout << "Compression ratio double for x: " << ((float)nr_of_vertices*8.f) / (float)nr_of_compressed_x_bytes << "\n";
  std::cout << "Compression ratio double for y: " << ((float)nr_of_vertices*8.f) / (float)nr_of_compressed_y_bytes << "\n";
  std::cout << "Compression ratio double for z: " << ((float)nr_of_vertices*8.f) / (float)nr_of_compressed_z_bytes << "\n";
  std::cout << "Total compression ratio double: " << ((float)nr_of_vertices*24.f) / (float)(nr_of_compressed_x_bytes + nr_of_compressed_y_bytes + nr_of_compressed_z_bytes) << "\n";

  trico_free(compressed_x);
  trico_free(decompressed_x);
  trico_free(decompressed_y);
  trico_free(compressed_y);
  trico_free(decompressed_z);
  trico_free(compressed_z);
  trico_free(x);
  trico_free(y);
  trico_free(z);
  trico_free(dx);
  trico_free(dy);
  trico_free(dz);
  trico_free(vertices);
  trico_free(triangles);
  }


void compress_vertices(const char* filename)
  {

  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

  trico_transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

  uint32_t nr_of_compressed_x_bytes;
  uint8_t* compressed_x;
  trico_compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices, 4, 10);

  uint32_t nr_of_floats;
  float* decompressed_x;
  trico_decompress(&nr_of_floats, &decompressed_x, compressed_x);

  TEST_EQ(nr_of_vertices, nr_of_floats);

  for (uint32_t i = 0; i < nr_of_floats; ++i)
    TEST_EQ(x[i], decompressed_x[i]);

  uint32_t nr_of_compressed_y_bytes;
  uint8_t* compressed_y;
  trico_compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices, 4, 10);

  float* decompressed_y;
  trico_decompress(&nr_of_floats, &decompressed_y, compressed_y);

  TEST_EQ(nr_of_vertices, nr_of_floats);

  for (uint32_t i = 0; i < nr_of_floats; ++i)
    TEST_EQ(y[i], decompressed_y[i]);

  uint32_t nr_of_compressed_z_bytes;
  uint8_t* compressed_z;
  trico_compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices, 4, 10);

  float* decompressed_z;
  trico_decompress(&nr_of_floats, &decompressed_z, compressed_z);

  TEST_EQ(nr_of_vertices, nr_of_floats);

  for (uint32_t i = 0; i < nr_of_floats; ++i)
    TEST_EQ(z[i], decompressed_z[i]);


  std::cout << "Compression ratio for x: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_x_bytes << "\n";
  std::cout << "Compression ratio for y: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_y_bytes << "\n";
  std::cout << "Compression ratio for z: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_z_bytes << "\n";
  std::cout << "Total compression ratio: " << ((float)nr_of_vertices*12.f) / (float)(nr_of_compressed_x_bytes + nr_of_compressed_y_bytes + nr_of_compressed_z_bytes) << "\n";

  trico_free(decompressed_x);
  trico_free(compressed_x);
  trico_free(decompressed_y);
  trico_free(compressed_y);
  trico_free(decompressed_z);
  trico_free(compressed_z);
  trico_free(x);
  trico_free(y);
  trico_free(z);
  trico_free(vertices);
  trico_free(triangles);
  }


void run_all_fps_compression_tests()
  {
  transpose_xyz_aos_to_soa("data/StanfordBunny.stl");
  compress_vertices("data/StanfordBunny.stl");
  compress_vertices_double("data/StanfordBunny.stl");
  }