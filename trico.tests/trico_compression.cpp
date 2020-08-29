#include "trico_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/trico.h>

#include <trico/iostl.h>

#include <iostream>
#include <fstream>

namespace trico
  {

  void test_header()
    {
    void* arch = open_archive_for_writing(1024);

    std::ofstream outfile;
    outfile.open("header.trc", std::ios::binary | std::ios::out);
    outfile.write((const char*)get_buffer_pointer(arch), get_size(arch));
    outfile.close();
    close_archive(arch);

    std::ifstream infile;
    infile.open("header.trc", std::ios::binary | std::ios::in);
    infile.seekg(0, std::ios::end);
    uint64_t length = (uint64_t)infile.tellg();
    infile.seekg(0, std::ios::beg);
    char *data = new char[length];
    infile.read(data, length);
    infile.close();

    TEST_EQ(8, length);

    arch = open_archive_for_reading((const uint8_t*)data, length);
    TEST_ASSERT(arch != nullptr);
    uint32_t version = get_version(arch);
    TEST_EQ(0, version);
    TEST_EQ(empty, get_next_stream_type(arch));
    close_archive(arch);

    delete[] data;
    }

  void test_stl(const char* filename)
    {
    std::cout << "Tests for file " << filename << "\n";
    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    void* arch = open_archive_for_writing(1024*1024);
    write_vertices(arch, nr_of_vertices, vertices);
    write_triangles(arch, nr_of_triangles, triangles);

    std::ofstream outfile;
    outfile.open("stltest.trc", std::ios::binary | std::ios::out);
    outfile.write((const char*)get_buffer_pointer(arch), get_size(arch));
    outfile.close();

    close_archive(arch);

    std::ifstream infile;
    infile.open("stltest.trc", std::ios::binary | std::ios::in);
    infile.seekg(0, std::ios::end);
    uint64_t length = (uint64_t)infile.tellg();
    infile.seekg(0, std::ios::beg);
    char *data = new char[length];
    infile.read(data, length);
    infile.close();

    arch = open_archive_for_reading((const uint8_t*)data, length);
    TEST_EQ(vertex_float_stream, get_next_stream_type(arch));

    uint32_t nr_vertices = get_number_of_vertices(arch);
    TEST_EQ(nr_of_vertices, nr_vertices);

    float* vertices_read = new float[nr_of_vertices * 3];

    TEST_ASSERT(read_vertices(arch, &vertices_read));
    for (uint32_t i = 0; i < nr_of_vertices * 3; ++i)
      {
      TEST_EQ(vertices[i], vertices_read[i]);
      }
    delete[] vertices_read;

    TEST_EQ(triangle_uint32_stream, get_next_stream_type(arch));
    uint32_t nr_triangles = get_number_of_triangles(arch);
    TEST_EQ(nr_of_triangles, nr_triangles);

    uint32_t* triangles_read = new uint32_t[nr_of_triangles * 3];
    TEST_ASSERT(read_triangles(arch, &triangles_read));
    for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
      {
      TEST_EQ(triangles[i], triangles_read[i]);
      }
    delete[] triangles_read;

    close_archive(arch);

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

    TEST_EQ(0, read_stl(&nr_of_vertices, &verticesf, &nr_of_triangles, &triangles32, filename));

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

    void* arch = open_archive_for_writing(1024 * 1024);
    write_vertices(arch, nr_of_vertices, vertices);
    write_triangles(arch, nr_of_triangles, triangles);

    uint64_t length = get_size(arch);
    char* data = new char[length];
    memcpy(data, get_buffer_pointer(arch), length);

    close_archive(arch);

    arch = open_archive_for_reading((const uint8_t*)data, length);
    TEST_EQ(vertex_double_stream, get_next_stream_type(arch));

    uint32_t nr_vertices = get_number_of_vertices(arch);
    TEST_EQ(nr_of_vertices, nr_vertices);

    double* vertices_read = new double[nr_of_vertices * 3];

    TEST_ASSERT(read_vertices(arch, &vertices_read));
    for (uint32_t i = 0; i < nr_of_vertices * 3; ++i)
      {
      TEST_EQ(vertices[i], vertices_read[i]);
      }
    delete[] vertices_read;

    TEST_EQ(triangle_uint64_stream, get_next_stream_type(arch));
    uint32_t nr_triangles = get_number_of_triangles(arch);
    TEST_EQ(nr_of_triangles, nr_triangles);

    uint64_t* triangles_read = new uint64_t[nr_of_triangles * 3];
    TEST_ASSERT(read_triangles(arch, &triangles_read));
    for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
      {
      TEST_EQ(triangles[i], triangles_read[i]);
      }
    delete[] triangles_read;

    close_archive(arch);

    delete[] vertices;
    delete[] triangles;

    delete[] data;
    std::cout << "********************************************\n";
    }

  }

void run_all_trico_compression_tests()
  {
  using namespace trico;
  test_header();
  test_stl("data/stanfordbunny.stl");
  test_stl_double_64("data/stanfordbunny.stl");
  }