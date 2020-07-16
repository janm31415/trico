#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/transpose_aos_to_soa.h>

#include <zlib/zlib.h>

#include <lz4/lz4.h>

#include <iostream>

namespace trico
  {


  void transpose_uint32_aos_to_soa(const char* filename)
    {
    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    uint8_t* b1;
    uint8_t* b2;
    uint8_t* b3;
    uint8_t* b4;

    transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, triangles, nr_of_triangles);

    uint32_t* triangles_from_b1b2b3b4;

    transpose_uint32_soa_to_aos(&triangles_from_b1b2b3b4, b1, b2, b3, b4, nr_of_triangles);

    for (uint32_t i = 0; i < nr_of_triangles; ++i)
      {
      TEST_EQ(triangles[i], triangles_from_b1b2b3b4[i]);
      }

    trico_free(triangles_from_b1b2b3b4);
    trico_free(b1);
    trico_free(b2);
    trico_free(b3);
    trico_free(b4);
    trico_free(vertices);
    trico_free(triangles);
    }

  void compress_triangles(const char* filename)
    {
    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    uint8_t* b1;
    uint8_t* b2;
    uint8_t* b3;
    uint8_t* b4;

    transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, triangles, nr_of_triangles);

    uint32_t total_length = 0;
    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    int bytes_written = LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles, (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for b1: " << ((float)nr_of_triangles) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }
    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    int bytes_written = LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles, (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for b2: " << ((float)nr_of_triangles) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }
    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    int bytes_written = LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles, (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for b3: " << ((float)nr_of_triangles) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }
    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    int bytes_written = LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles, (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for b4: " << ((float)nr_of_triangles) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }

    std::cout << "Compression ratio lz4 total: " << ((float)nr_of_triangles*4.f) / (float)total_length << "\n";


    uint32_t* triangles_from_b1b2b3b4;

    transpose_uint32_soa_to_aos(&triangles_from_b1b2b3b4, b1, b2, b3, b4, nr_of_triangles);

    trico_free(triangles_from_b1b2b3b4);
    trico_free(b1);
    trico_free(b2);
    trico_free(b3);
    trico_free(b4);
    trico_free(vertices);
    trico_free(triangles);
    }

  void test_int_compression(const char* filename)
    {
    std::cout << "Tests for file " << filename << "\n";
    transpose_uint32_aos_to_soa(filename);
    compress_triangles(filename);
    }
  }


void run_all_int_compression_tests()
  {
  using namespace trico;

  //test_int_compression("data/StanfordBunny.stl");  
  test_int_compression("D:/stl/kouros.stl");
  }