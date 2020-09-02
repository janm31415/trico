#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/transpose_aos_to_soa.h>

#include <trico_io/iostl.h>

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


void transpose_uint32_aos_to_soa(const char* filename)
  {
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);


  tic();
  trico_transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, triangles, nr_of_triangles * 3);
  toc("transpose_uint32_aos_to_soa time: ");


  uint32_t* triangles_from_b1b2b3b4 = (uint32_t*)trico_malloc(nr_of_triangles * 3 * sizeof(uint32_t));

  tic();
  trico_transpose_uint32_soa_to_aos(&triangles_from_b1b2b3b4, b1, b2, b3, b4, nr_of_triangles * 3);
  toc("transpose_uint32_soa_to_aos time: ");

  for (uint32_t i = 0; i < nr_of_triangles * 3; ++i)
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

void compress_triangles_lz4(const char* filename)
  {
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);

  trico_transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, triangles, nr_of_triangles * 3);

  uint32_t total_length = 0;
  {
  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = new uint8_t[estimateLen];

  int bytes_written = LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  total_length += bytes_written;
  std::cout << "Compression ratio lz4 for b1: " << ((float)nr_of_triangles * 3) / (float)bytes_written << "\n";

  delete[] compressed_buf;
  }
  {
  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = new uint8_t[estimateLen];

  int bytes_written = LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  total_length += bytes_written;
  std::cout << "Compression ratio lz4 for b2: " << ((float)nr_of_triangles * 3) / (float)bytes_written << "\n";

  delete[] compressed_buf;
  }
  {
  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = new uint8_t[estimateLen];

  int bytes_written = LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  total_length += bytes_written;
  std::cout << "Compression ratio lz4 for b3: " << ((float)nr_of_triangles * 3) / (float)bytes_written << "\n";

  delete[] compressed_buf;
  }
  {
  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = new uint8_t[estimateLen];

  int bytes_written = LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  total_length += bytes_written;
  std::cout << "Compression ratio lz4 for b4: " << ((float)nr_of_triangles * 3) / (float)bytes_written << "\n";

  delete[] compressed_buf;
  }

  std::cout << "Compression ratio lz4 total: " << ((float)nr_of_triangles*12.f) / (float)total_length << "\n";


  trico_free(b1);
  trico_free(b2);
  trico_free(b3);
  trico_free(b4);
  trico_free(vertices);
  trico_free(triangles);
  }

void compress_triangles_lz4_no_shuffling(const char* filename)
  {
  uint32_t nr_of_vertices;
  float* vertices;
  uint32_t nr_of_triangles;
  uint32_t* triangles;

  TEST_EQ(1, trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

  uint32_t total_length = 0;
  {
  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 12);
  uint8_t* compressed_buf = new uint8_t[estimateLen];

  int bytes_written = LZ4_compress_default((const char*)triangles, (char*)compressed_buf, (int)nr_of_triangles * 12, (int)estimateLen);
  total_length += bytes_written;
  std::cout << "Compression ratio lz4 for triangles unshuffled: " << ((float)nr_of_triangles * 12) / (float)bytes_written << "\n";

  delete[] compressed_buf;
  }

  trico_free(vertices);
  trico_free(triangles);
  }

void test_int_compression(const char* filename)
  {
  transpose_uint32_aos_to_soa(filename);
  compress_triangles_lz4(filename);
  compress_triangles_lz4_no_shuffling(filename);
  }



void run_all_int_compression_tests()
  {
  test_int_compression("data/StanfordBunny.stl");  
  }