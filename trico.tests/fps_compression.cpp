#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/transpose_aos_to_soa.h>
#include <trico/floating_point_stream_compression.h>

#include <zlib/zlib.h>

#include <lz4/lz4.h>

#include <iostream>

#include "timer.h"


namespace trico
  {
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

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    tic();
    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);
    toc("transpose_xyz_aos_to_soa time: ");

    for (uint32_t i = 0; i < nr_of_vertices; ++i)
      {
      TEST_EQ(x[i], vertices[i * 3]);
      TEST_EQ(y[i], vertices[i * 3 + 1]);
      TEST_EQ(z[i], vertices[i * 3 + 2]);
      }

    float* vertices_from_xyz = (float*)trico_malloc(sizeof(float)*nr_of_vertices * 3);

    tic();
    transpose_xyz_soa_to_aos(&vertices_from_xyz, x, y, z, nr_of_vertices);
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

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

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
    compress(&nr_of_compressed_x_bytes, &compressed_x, dx, nr_of_vertices);
    
    uint32_t nr_of_doubles;
    double* decompressed_x;
    decompress(&nr_of_doubles, &decompressed_x, compressed_x);

    TEST_EQ(nr_of_vertices, nr_of_doubles);

    for (uint32_t i = 0; i < nr_of_doubles; ++i)
      TEST_EQ(dx[i], decompressed_x[i]);
      
    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, dy, nr_of_vertices);
    
    double* decompressed_y;
    decompress(&nr_of_doubles, &decompressed_y, compressed_y);

    TEST_EQ(nr_of_vertices, nr_of_doubles);

    for (uint32_t i = 0; i < nr_of_doubles; ++i)
      TEST_EQ(dy[i], decompressed_y[i]);
      
    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;
    compress(&nr_of_compressed_z_bytes, &compressed_z, dz, nr_of_vertices);
    
    double* decompressed_z;
    decompress(&nr_of_doubles, &decompressed_z, compressed_z);

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

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    uint32_t nr_of_compressed_x_bytes;
    uint8_t* compressed_x;
    compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices);
    
    uint32_t nr_of_floats;
    float* decompressed_x;
    decompress(&nr_of_floats, &decompressed_x, compressed_x);

    TEST_EQ(nr_of_vertices, nr_of_floats);

    for (uint32_t i = 0; i < nr_of_floats; ++i)
      TEST_EQ(x[i], decompressed_x[i]);
      
    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices);
    
    float* decompressed_y;
    decompress(&nr_of_floats, &decompressed_y, compressed_y);

    TEST_EQ(nr_of_vertices, nr_of_floats);

    for (uint32_t i = 0; i < nr_of_floats; ++i)
      TEST_EQ(y[i], decompressed_y[i]);
      
    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;
    compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices);
    
    float* decompressed_z;
    decompress(&nr_of_floats, &decompressed_z, compressed_z);

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

  void compress_vertices_and_zlib(const char* filename)
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    uint32_t nr_of_compressed_x_bytes;
    uint8_t* compressed_x;
    compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices);

    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)compressed_x;
    defstream.avail_in = nr_of_compressed_x_bytes;
    defstream.next_out = 0;
    defstream.avail_out = 0;

    uint32_t total_length = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_compressed_x_bytes);
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio fpc2 + zlib for x: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);
  

    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices);

    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)compressed_y;
    defstream.avail_in = nr_of_compressed_y_bytes;
    defstream.next_out = 0;
    defstream.avail_out = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_compressed_y_bytes);
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio fpc2 + zlib for y: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;
    compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices);

    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)compressed_z;
    defstream.avail_in = nr_of_compressed_z_bytes;
    defstream.next_out = 0;
    defstream.avail_out = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_compressed_z_bytes);
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio fpc2 + zlib for z: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    std::cout << "Total compression ratio: " << ((float)nr_of_vertices*12.f) / (float)(total_length) << "\n";

    trico_free(compressed_x);
    trico_free(compressed_y);
    trico_free(compressed_z);
    trico_free(x);
    trico_free(y);
    trico_free(z);
    trico_free(vertices);
    trico_free(triangles);
    }

  void compress_vertices_no_swizzling(const char* filename)
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;
    
    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));


    uint32_t nr_of_compressed_bytes;
    uint8_t* compressed;
    compress(&nr_of_compressed_bytes, &compressed, vertices, nr_of_vertices * 3);

    uint32_t nr_of_floats;
    float* decompressed;
    decompress(&nr_of_floats, &decompressed, compressed);

    TEST_EQ(nr_of_vertices * 3, nr_of_floats);

    for (uint32_t i = 0; i < nr_of_floats; ++i)
      TEST_EQ(vertices[i], decompressed[i]);

    std::cout << "Total compression ratio (no swizzling): " << ((float)nr_of_vertices*12.f) / (float)(nr_of_compressed_bytes) << "\n";

    trico_free(decompressed);
    trico_free(compressed);
    trico_free(vertices);
    trico_free(triangles);
    }

  void compress_vertices_zlib(const char* filename)
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;
    
    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)x;
    defstream.avail_in = nr_of_vertices * sizeof(float);
    defstream.next_out = 0;
    defstream.avail_out = 0;

    uint32_t total_length = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_vertices * sizeof(float));
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio zlib for x: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)y;
    defstream.avail_in = nr_of_vertices * sizeof(float);
    defstream.next_out = 0;
    defstream.avail_out = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_vertices * sizeof(float));
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio zlib for y: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)z;
    defstream.avail_in = nr_of_vertices * sizeof(float);
    defstream.next_out = 0;
    defstream.avail_out = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_vertices * sizeof(float));
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;
      total_length += (uint32_t)length;
      std::cout << "Compression ratio zlib for z: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    std::cout << "Compression ratio zlib total: " << ((float)nr_of_vertices*12.f) / (float)total_length << "\n";

    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)vertices;
    defstream.avail_in = nr_of_vertices * 3 * sizeof(float);
    defstream.next_out = 0;
    defstream.avail_out = 0;

    if (deflateInit(&defstream, Z_BEST_COMPRESSION) == Z_OK)
      {
      auto estimateLen = deflateBound(&defstream, nr_of_vertices * 3 * sizeof(float));
      uint8_t* compressed_buf = new uint8_t[estimateLen];
      defstream.next_out = compressed_buf;
      defstream.avail_out = estimateLen;
      deflate(&defstream, Z_FINISH);


      std::streamsize length = (uint8_t*)defstream.next_out - compressed_buf;

      std::cout << "Compression ratio zlib for vertices: " << ((float)nr_of_vertices*12.f) / (float)length << "\n";

      delete[] compressed_buf;
      }
    deflateEnd(&defstream);

    trico_free(x);
    trico_free(y);
    trico_free(z);
    trico_free(vertices);
    trico_free(triangles);
    }



  void compress_vertices_lz4(const char* filename)
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;
    
    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename));

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);

    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);
    uint32_t total_length = 0;
    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_vertices * sizeof(float));
    uint8_t* compressed_buf = new uint8_t[estimateLen];
    
    int bytes_written = LZ4_compress_default((const char*)x, (char*)compressed_buf, (int)nr_of_vertices * sizeof(float), (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for x: " << ((float)nr_of_vertices*4.f) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }

    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_vertices * sizeof(float));
    uint8_t* compressed_buf = new uint8_t[estimateLen];
    
    int bytes_written = LZ4_compress_default((const char*)y, (char*)compressed_buf, (int)nr_of_vertices * sizeof(float), (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for y: " << ((float)nr_of_vertices*4.f) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }

    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_vertices * sizeof(float));
    uint8_t* compressed_buf = new uint8_t[estimateLen];
    
    int bytes_written = LZ4_compress_default((const char*)z, (char*)compressed_buf, (int)nr_of_vertices * sizeof(float), (int)estimateLen);
    total_length += bytes_written;
    std::cout << "Compression ratio lz4 for z: " << ((float)nr_of_vertices*4.f) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }

    std::cout << "Compression ratio lz4 total: " << ((float)nr_of_vertices*12.f) / (float)total_length << "\n";

    {
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_vertices * 3 * sizeof(float));
    uint8_t* compressed_buf = new uint8_t[estimateLen];
    
    int bytes_written = LZ4_compress_default((const char*)vertices, (char*)compressed_buf, 3 * (int)nr_of_vertices * sizeof(float), (int)estimateLen);

    std::cout << "Compression ratio lz4 for vertices: " << ((float)nr_of_vertices*12.f) / (float)bytes_written << "\n";

    delete[] compressed_buf;
    }    

    trico_free(x);
    trico_free(y);
    trico_free(z);
    trico_free(vertices);
    trico_free(triangles);
    }

  void test_float_compression(const char* filename)
    {
    std::cout << "Tests for file " << filename << "\n";
    transpose_xyz_aos_to_soa(filename);    
    compress_vertices(filename);
    compress_vertices_no_swizzling(filename);
    compress_vertices_double(filename);
    
    //compress_vertices(filename);
    
    
    //compress_vertices_zlib(filename);
    //compress_vertices_2_and_zlib(filename);
    //compress_vertices_lz4(filename);
    std::cout << "********************************************\n";
    }

  void test_double_compression(const char* filename)
    {
    std::cout << "Tests for file " << filename << "\n";   
    compress_vertices_double(filename);
    std::cout << "********************************************\n";
    }
  }

void run_all_fps_compression_tests()
  {
  using namespace trico;
  transpose_xyz_aos_to_soa("D:/stl/kouros.stl");
  test_float_compression("data/StanfordBunny.stl");  
  test_double_compression("data/StanfordBunny.stl");
  
  //test_float_compression("D:/stl/dino.stl");
  //test_float_compression("D:/stl/bad.stl");
  /*
  test_compression("D:/stl/horned_sea_star.stl");
  test_compression("D:/stl/core.stl");
  test_compression("D:/stl/pega.stl");
  test_compression("D:/stl/kouros.stl");
  test_compression("D:/stl/Aston Martin DB9 sell.stl");
  test_compression("D:/stl/front-cover.stl");
  test_compression("D:/stl/front.stl");
  test_compression("D:/stl/einstein.stl");
  test_compression("D:/stl/ima_test_tank.stl");
  test_compression("D:/stl/jan.stl");
  test_compression("D:/stl/SKIWI.stl");
  test_compression("D:/stl/wasp_bot.stl");  
  test_compression("D:/stl/RobotRed.stl");
  */
  
  //test_compression_double("data/StanfordBunny.stl");    
  /*
  test_compression_double("D:/stl/dino.stl");
  test_compression_double("D:/stl/bad.stl");
  test_compression_double("D:/stl/horned_sea_star.stl");
  test_compression_double("D:/stl/core.stl");
  test_compression_double("D:/stl/pega.stl");
  test_compression_double("D:/stl/kouros.stl");
  test_compression_double("D:/stl/Aston Martin DB9 sell.stl");
  test_compression_double("D:/stl/front-cover.stl");
  test_compression_double("D:/stl/front.stl");
  test_compression_double("D:/stl/einstein.stl");
  test_compression_double("D:/stl/ima_test_tank.stl");
  test_compression_double("D:/stl/jan.stl");
  test_compression_double("D:/stl/SKIWI.stl");
  test_compression_double("D:/stl/wasp_bot.stl");
  test_compression_double("D:/stl/RobotRed.stl");
  */
  }