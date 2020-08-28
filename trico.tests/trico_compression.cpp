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
    void* arch = open_writable_archive("header.trc");
    close_archive(arch);

    arch = open_readable_archive("header.trc");
    TEST_ASSERT(arch != nullptr);
    uint32_t version = get_version(arch);
    TEST_EQ(0, version);
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

    void* arch = open_writable_archive("stltest.trc");
    write_vertices(arch, vertices, nr_of_vertices);
    write_triangles(arch, triangles, nr_of_triangles);
    close_archive(arch);
    trico_free(vertices);
    trico_free(triangles);

    std::cout << "********************************************\n";
    }
  }



void run_all_trico_compression_tests()
  {
  using namespace trico;
  test_header();
  test_stl("data/StanfordBunny.stl");
  }