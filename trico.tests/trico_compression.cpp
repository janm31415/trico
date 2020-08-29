#include "trico_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/trico.h>

#include <trico/iostl.h>

#include <iostream>

namespace trico
  {

  void test_header()
    {
    void* arch = open_archive_for_writing("header.trc");
    close_archive(arch);

    arch = open_archive_for_reading("header.trc");
    TEST_ASSERT(arch != nullptr);
    uint32_t version = get_version(arch);
    TEST_EQ(0, version);
    TEST_EQ(empty, get_next_stream_type(arch));
    close_archive(arch);
    }

  void test_stl(const char* filename)
    {
    std::cout << "Tests for file " << filename << "\n";
    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    void* arch = open_archive_for_writing("stltest.trc");
    write_vertices(arch, nr_of_vertices, vertices);
    write_triangles(arch, nr_of_triangles, triangles);
    close_archive(arch);  

    arch = open_archive_for_reading("stltest.trc");
    TEST_EQ(vertex_float_stream, get_next_stream_type(arch));
    
    float* vertices_read;
    uint32_t nr_of_vertices_read;
    TEST_ASSERT(read_vertices(arch, &nr_of_vertices_read, &vertices_read));

    TEST_EQ(nr_of_vertices, nr_of_vertices_read);

    for (uint32_t i = 0; i < nr_of_vertices*3; ++i)
      {
      TEST_EQ(vertices[i], vertices_read[i]);
      }

    trico_free(vertices_read);
    trico_free(vertices);

    TEST_EQ(triangle_uint32_stream, get_next_stream_type(arch));

    uint32_t* triangles_read;
    uint32_t nr_of_triangles_read;
    TEST_ASSERT(read_triangles(arch, &nr_of_triangles_read, &triangles_read));

    TEST_EQ(nr_of_triangles, nr_of_triangles_read);

    for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
      {
      TEST_EQ(triangles[i], triangles_read[i]);
      }

    trico_free(triangles_read);
    trico_free(triangles);

    TEST_EQ(empty, get_next_stream_type(arch));

    close_archive(arch);


    std::cout << "********************************************\n";
    }
  }



void run_all_trico_compression_tests()
  {
  using namespace trico;
  test_header();
  test_stl("data/StanfordBunny.stl");
  //test_stl("D:/stl/kouros.stl");
  }