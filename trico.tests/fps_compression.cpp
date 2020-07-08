#include "fps_compression.h"
#include "test_assert.h"

#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/transpose_aos_to_soa.h>
#include <trico/floating_point_stream_compression.h>

#include <zlib/zlib.h>

#include <iostream>

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

  void compress_vertices_standford_bunny()
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "data/StanfordBunny.stl"));
    //TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "D:/stl/kouros.stl"));
    float* x;
    float* y;
    float* z;

    transpose_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

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

    //TEST_EQ(13399, nr_of_compressed_x_bytes);
    //TEST_EQ(115586, nr_of_compressed_y_bytes);
    //TEST_EQ(125249, nr_of_compressed_z_bytes);

    std::cout << "Compression ratio for x: " << ((float)nr_of_vertices*4.f) /(float)nr_of_compressed_x_bytes << "\n";
    std::cout << "Compression ratio for y: " << ((float)nr_of_vertices*4.f) /(float)nr_of_compressed_y_bytes << "\n";
    std::cout << "Compression ratio for z: " << ((float)nr_of_vertices*4.f) /(float)nr_of_compressed_z_bytes << "\n";
    std::cout << "Total compression ratio: " << ((float)nr_of_vertices*12.f) /(float)(nr_of_compressed_x_bytes + nr_of_compressed_y_bytes + nr_of_compressed_z_bytes) << "\n";

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

  void compress_vertices_standford_bunny_no_swizzling()
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "data/StanfordBunny.stl"));
    //TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "D:/stl/kouros.stl"));
    

    uint32_t nr_of_compressed_bytes;
    uint8_t* compressed;
    compress(&nr_of_compressed_bytes, &compressed, vertices, nr_of_vertices*3);

    uint32_t nr_of_floats;
    float* decompressed;
    decompress(&nr_of_floats, &decompressed, compressed);

    TEST_EQ(nr_of_vertices*3, nr_of_floats);

    for (uint32_t i = 0; i < nr_of_floats; ++i)
      TEST_EQ(vertices[i], decompressed[i]);

    std::cout << "Total compression ratio (no swizzling): " << ((float)nr_of_vertices*12.f) / (float)(nr_of_compressed_bytes) << "\n";

    trico_free(decompressed);
    trico_free(compressed);
    trico_free(vertices);
    trico_free(triangles);
    }

  void compress_vertices_standford_bunny_zlib()
    {

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

    TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "data/StanfordBunny.stl"));
    //TEST_EQ(0, read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "D:/stl/kouros.stl"));
    float* x;
    float* y;
    float* z;

    transpose_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.next_in = (uint8_t*)x;
    defstream.avail_in = nr_of_vertices*sizeof(float);
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
      total_length += length;
      std::cout << "Compression ratio zlib for x: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }

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
      total_length += length;
      std::cout << "Compression ratio zlib for y: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }

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
      total_length += length;
      std::cout << "Compression ratio zlib for z: " << ((float)nr_of_vertices*4.f) / (float)length << "\n";

      delete[] compressed_buf;
      }

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


    //std::cout << "Compression ratio for x: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_x_bytes << "\n";
    //std::cout << "Compression ratio for y: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_y_bytes << "\n";
    //std::cout << "Compression ratio for z: " << ((float)nr_of_vertices*4.f) / (float)nr_of_compressed_z_bytes << "\n";
    //std::cout << "Total compression ratio: " << ((float)nr_of_vertices*12.f) / (float)(nr_of_compressed_x_bytes + nr_of_compressed_y_bytes + nr_of_compressed_z_bytes) << "\n";


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
  compress_vertices_standford_bunny();
  compress_vertices_standford_bunny_no_swizzling();
  compress_vertices_standford_bunny_zlib();
  }