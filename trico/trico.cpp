#include "trico.h"
#include "transpose_aos_to_soa.h"
#include "floating_point_stream_compression.h"
#include "alloc.h"
#include <iostream>
#include <cassert>

#include <lz4/lz4.h>

namespace trico
  {  

  struct archive
    {
    archive() : version(0), next_stream_type(empty) {}
    FILE* f;
    uint32_t version;
    e_stream_type next_stream_type;
    };

  namespace
    {

    void write_header(archive* arch)
      {
      uint32_t Trco = 0x6f637254;
      fwrite(&Trco, sizeof(uint32_t), 1, arch->f);
      fwrite(&(arch->version), sizeof(uint32_t), 1, arch->f);
      }

    void read_next_stream_type(archive* arch)
      {
      if (!feof(arch->f))
        fread(&(arch->next_stream_type), 1, 1, arch->f); // version number
      }

    bool read_header(archive* arch)
      {
      uint32_t Trco;
      fread(&Trco, sizeof(uint32_t), 1, arch->f);
      if (Trco != 0x6f637254) // not a trico file
        {
        return false;
        }
      fread(&(arch->version), sizeof(uint32_t), 1, arch->f); // version number
      read_next_stream_type(arch);
      return true;
      }
    }

  void* open_writable_archive(const char* filename)
    {
    archive* arch = new archive();
    arch->f = fopen(filename, "wb");
    if (!arch->f)
      {
      delete arch;
      return nullptr;
      }
    write_header(arch);
    return arch;
    }

  void* open_readable_archive(const char* filename)
    {
    archive* arch = new archive();
    arch->f = fopen(filename, "rb");
    if (!arch->f || !read_header(arch))
      {
      delete arch;
      return nullptr;
      }
    return arch;
    }

  void close_archive(void* a)
    {
    archive* arch = (archive*)a;
    if (arch->f)
      fclose(arch->f);
    delete arch;
    }

  uint32_t get_version(void* a)
    {
    archive* arch = (archive*)a;
    return arch->version;
    }

  void write_vertices(void* a, const float* vertices, uint32_t nr_of_vertices)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)vertex_float_stream;
    fwrite(&header, 1, 1, arch->f);

    float* x;
    float* y;
    float* z;
    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    uint32_t nr_of_compressed_x_bytes;
    uint8_t* compressed_x;
    compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices);

    trico_free(x);
    fwrite(&nr_of_compressed_x_bytes, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_x, 1, nr_of_compressed_x_bytes, arch->f);
    trico_free(compressed_x);

    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices);

    trico_free(y);
    fwrite(&nr_of_compressed_y_bytes, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_y, 1, nr_of_compressed_y_bytes, arch->f);
    trico_free(compressed_y);

    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;    
    compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices);

    trico_free(z);
    fwrite(&nr_of_compressed_z_bytes, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_z, 1, nr_of_compressed_z_bytes, arch->f);
    trico_free(compressed_z);
    }

  void write_triangles(void* a, const uint32_t* tria_indices, uint32_t nr_of_triangles)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)triangle_uint32_stream;
    fwrite(&header, 1, 1, arch->f);

    uint8_t* b1;
    uint8_t* b2;
    uint8_t* b3;
    uint8_t* b4;

    transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, tria_indices, nr_of_triangles * 3);

    
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);    
    fwrite(&bytes_written, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_buf, 1, bytes_written, arch->f);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    fwrite(&bytes_written, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_buf, 1, bytes_written, arch->f);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    fwrite(&bytes_written, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_buf, 1, bytes_written, arch->f);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    fwrite(&bytes_written, sizeof(uint32_t), 1, arch->f);
    fwrite(compressed_buf, 1, bytes_written, arch->f);

    delete[] compressed_buf;
    


    trico_free(b1);
    trico_free(b2);
    trico_free(b3);
    trico_free(b4);
    }

  e_stream_type get_next_stream_type(void* a)
    {
    archive* arch = (archive*)a;
    return arch->next_stream_type;
    }

  bool read_vertices(void* a, uint32_t* number_of_vertices, float** vertices)
    {
    archive* arch = (archive*)a;
    *number_of_vertices = 0;
    *vertices = nullptr;
    if (get_next_stream_type(arch) != vertex_float_stream)
      return false;

    uint32_t nr_of_compressed_bytes;
    fread(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch->f);
    void* compressed = trico_malloc(nr_of_compressed_bytes);
    fread(compressed, 1, nr_of_compressed_bytes, arch->f);
    float* decompressed_x;
    uint32_t nr_of_floats_x;
    decompress(&nr_of_floats_x, &decompressed_x, (const uint8_t*)compressed);
    
    fread(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch->f);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    fread(compressed, 1, nr_of_compressed_bytes, arch->f);
    float* decompressed_y;
    uint32_t nr_of_floats_y;
    decompress(&nr_of_floats_y, &decompressed_y, (const uint8_t*)compressed);

    fread(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch->f);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    fread(compressed, 1, nr_of_compressed_bytes, arch->f);
    float* decompressed_z;
    uint32_t nr_of_floats_z;
    decompress(&nr_of_floats_z, &decompressed_z, (const uint8_t*)compressed);
    trico_free(compressed);

    assert(nr_of_floats_x == nr_of_floats_y);
    assert(nr_of_floats_x == nr_of_floats_z);

    *number_of_vertices = nr_of_floats_x;
    transpose_xyz_soa_to_aos(vertices, decompressed_x, decompressed_y, decompressed_z, *number_of_vertices);

    trico_free(decompressed_x);
    trico_free(decompressed_y);
    trico_free(decompressed_z);

    read_next_stream_type(arch);

    return true;
    }
  }