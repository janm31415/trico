#include "trico_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/trico.h>

#include <trico/iostl.h>

#include <iostream>
#include <fstream>

#include <cstring>

void test_header()
  {
  void* arch = trico_open_archive_for_writing(1024);

  std::ofstream outfile;
  outfile.open("header.trc", std::ios::binary | std::ios::out);
  outfile.write((const char*)trico_get_buffer_pointer(arch), trico_get_size(arch));
  outfile.close();
  trico_close_archive(arch);

  std::ifstream infile;
  infile.open("header.trc", std::ios::binary | std::ios::in);
  infile.seekg(0, std::ios::end);
  uint64_t length = (uint64_t)infile.tellg();
  infile.seekg(0, std::ios::beg);
  char *data = new char[length];
  infile.read(data, length);
  infile.close();

  TEST_EQ(8, length);

  arch = trico_open_archive_for_reading((const uint8_t*)data, length);
  TEST_ASSERT(arch != nullptr);
  uint32_t version = trico_get_version(arch);
  TEST_EQ(0, version);
  TEST_EQ(empty, trico_get_next_stream_type(arch));
  trico_close_archive(arch);

  delete[] data;
  }

void test_stl(const char* filename)
  {
  std::cout << "Tests for file " << filename << "\n";
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  void* arch = trico_open_archive_for_writing(1024 * 1024);
  TEST_ASSERT(trico_write_vertices(arch, nr_of_vertices, vertices));
  TEST_ASSERT(trico_write_triangles(arch, nr_of_triangles, triangles));

  std::ofstream outfile;
  outfile.open("stltest.trc", std::ios::binary | std::ios::out);
  outfile.write((const char*)trico_get_buffer_pointer(arch), trico_get_size(arch));
  outfile.close();

  trico_close_archive(arch);

  std::ifstream infile;
  infile.open("stltest.trc", std::ios::binary | std::ios::in);
  infile.seekg(0, std::ios::end);
  uint64_t length = (uint64_t)infile.tellg();
  infile.seekg(0, std::ios::beg);
  char *data = new char[length];
  infile.read(data, length);
  infile.close();

  arch = trico_open_archive_for_reading((const uint8_t*)data, length);
  TEST_EQ(vertex_float_stream, trico_get_next_stream_type(arch));

  uint32_t nr_vertices = trico_get_number_of_vertices(arch);
  TEST_EQ(nr_of_vertices, nr_vertices);

  float* vertices_read = new float[nr_of_vertices * 3];

  TEST_ASSERT(trico_read_vertices(arch, &vertices_read));
  for (uint32_t i = 0; i < nr_of_vertices * 3; ++i)
    {
    TEST_EQ(vertices[i], vertices_read[i]);
    }
  delete[] vertices_read;

  TEST_EQ(triangle_uint32_stream, trico_get_next_stream_type(arch));
  uint32_t nr_triangles = trico_get_number_of_triangles(arch);
  TEST_EQ(nr_of_triangles, nr_triangles);

  uint32_t* triangles_read = new uint32_t[nr_of_triangles * 3];
  TEST_ASSERT(trico_read_triangles(arch, &triangles_read));
  for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
    {
    TEST_EQ(triangles[i], triangles_read[i]);
    }
  delete[] triangles_read;

  trico_close_archive(arch);

  trico_free(vertices);
  trico_free(triangles);

  delete[] data;
  std::cout << "********************************************\n";
  }


void test_stl_double_64(const char* filename)
  {
  std::cout << "Tests for file " << filename << "\n";
  uint32_t nr_of_vertices;
  float* verticesf;
  uint32_t nr_of_triangles;
  uint32_t* triangles32;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &verticesf, &nr_of_triangles, &triangles32, filename));

  double* vertices = new double[nr_of_vertices * 3];
  uint64_t* triangles = new uint64_t[nr_of_triangles * 3];

  for (uint32_t i = 0; i < nr_of_vertices * 3; ++i)
    {
    vertices[i] = (double)verticesf[i];
    }
  for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
    {
    triangles[i] = (uint64_t)triangles32[i];
    }

  trico_free(verticesf);
  trico_free(triangles32);

  void* arch = trico_open_archive_for_writing(1024 * 1024);
  TEST_ASSERT(trico_write_vertices_double(arch, nr_of_vertices, vertices));
  TEST_ASSERT(trico_write_triangles_long(arch, nr_of_triangles, triangles));

  uint64_t length = trico_get_size(arch);
  char* data = new char[length];
  memcpy(data, trico_get_buffer_pointer(arch), length);

  trico_close_archive(arch);

  arch = trico_open_archive_for_reading((const uint8_t*)data, length);
  TEST_EQ(vertex_double_stream, trico_get_next_stream_type(arch));

  uint32_t nr_vertices = trico_get_number_of_vertices(arch);
  TEST_EQ(nr_of_vertices, nr_vertices);

  double* vertices_read = new double[nr_of_vertices * 3];

  TEST_ASSERT(trico_read_vertices_double(arch, &vertices_read));
  for (uint32_t i = 0; i < nr_of_vertices * 3; ++i)
    {
    TEST_EQ(vertices[i], vertices_read[i]);
    }
  delete[] vertices_read;

  TEST_EQ(triangle_uint64_stream, trico_get_next_stream_type(arch));
  uint32_t nr_triangles = trico_get_number_of_triangles(arch);
  TEST_EQ(nr_of_triangles, nr_triangles);

  uint64_t* triangles_read = new uint64_t[nr_of_triangles * 3];
  TEST_ASSERT(trico_read_triangles_long(arch, &triangles_read));
  for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
    {
    TEST_EQ(triangles[i], triangles_read[i]);
    }
  delete[] triangles_read;

  trico_close_archive(arch);

  delete[] vertices;
  delete[] triangles;

  delete[] data;
  std::cout << "********************************************\n";
  }


void run_all_trico_compression_tests()
  {
  test_header();
  test_stl("data/stanfordbunny.stl");
  test_stl_double_64("data/stanfordbunny.stl");
  }