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
    archive() : buffer(nullptr), buffer_pointer(nullptr), data(nullptr), data_pointer(nullptr), version(0), next_stream_type(empty), buffer_size(0), data_size(0), size_available(0), writable(false) {}
    uint8_t* buffer;
    uint8_t* buffer_pointer;
    const uint8_t* data;
    const uint8_t* data_pointer;
    uint32_t version;
    e_stream_type next_stream_type;
    uint64_t buffer_size;
    uint64_t data_size;
    uint64_t size_available;
    bool writable;
    };

  namespace
    {

    bool sufficient_buffer_available(archive* arch, uint64_t bytes_needed)
      {
      return arch->size_available >= bytes_needed;
      }

    bool buffer_ready_for_writing(archive* arch, uint64_t bytes_needed)
      {
      if (!arch->writable)
        throw std::runtime_error("archive is read-only");
      if (!sufficient_buffer_available(arch, bytes_needed))
        {
        uint64_t extra_size = bytes_needed - arch->size_available;
        auto buff_diff = arch->buffer_pointer - arch->buffer;
        arch->buffer = (uint8_t*)trico_realloc(arch->buffer, arch->buffer_size + extra_size);
        if (arch->buffer == nullptr)
          return false;
        arch->buffer_pointer = arch->buffer + buff_diff;
        arch->buffer_size += extra_size;
        arch->size_available += extra_size;
        }
      return true;
      }

    void write_unsafe(const void* buf, uint64_t element_size, uint64_t element_count, archive* arch)
      {
      uint64_t sz = element_size * element_count;
      memcpy(arch->buffer_pointer, buf, sz);
      arch->buffer_pointer += sz;
      arch->size_available -= sz;
      }

    void write(const void* buf, uint64_t element_size, uint64_t element_count, archive* arch)
      {
      if (!buffer_ready_for_writing(arch, element_size * element_count))
        throw std::runtime_error("out of memory");
      write_unsafe(buf, element_size, element_count, arch);
      }

    void read(void* buf, uint64_t element_size, uint64_t element_count, archive* arch)
      {
      if (arch->writable)
        throw std::runtime_error("archive is write-only");
      uint64_t sz = element_size * element_count;
      uint64_t data_read = arch->data_pointer - arch->data;
      if ((data_read + sz) > arch->data_size)
        throw std::runtime_error("out of bounds");
      memcpy(buf, arch->data_pointer, sz);
      arch->data_pointer += sz;
      }


    void read_inplace(void* buf, uint64_t element_size, uint64_t element_count, archive* arch)
      {
      if (arch->writable)
        throw std::runtime_error("archive is write-only");
      uint64_t sz = element_size * element_count;
      uint64_t data_read = arch->data_pointer - arch->data;
      if ((data_read + sz) > arch->data_size)
        throw std::runtime_error("out of bounds");
      memcpy(buf, arch->data_pointer, sz);
      }

    bool write_header(archive* arch)
      {
      if (!buffer_ready_for_writing(arch, 8))
        return false;
      uint32_t Trco = 0x6f637254;
      write_unsafe(&Trco, sizeof(uint32_t), 1, arch);
      write_unsafe(&(arch->version), sizeof(uint32_t), 1, arch);
      return true;
      }

    void read_next_stream_type(archive* arch)
      {
      assert(!arch->writable);
      if ((uint64_t)(arch->data_pointer - arch->data) < arch->data_size)
        {
        read(&(arch->next_stream_type), 1, 1, arch);
        }
      else
        arch->next_stream_type = empty;
      }

    bool read_header(archive* arch)
      {
      uint32_t Trco;
      read(&Trco, sizeof(uint32_t), 1, arch);
      if (Trco != 0x6f637254) // not a trico file
        {
        return false;
        }
      read(&(arch->version), sizeof(uint32_t), 1, arch); // version number
      read_next_stream_type(arch);
      return true;
      }
    }

  void* open_archive_for_writing(uint64_t initial_buffer_size)
    {
    archive* arch = new archive();
    arch->buffer = (uint8_t*)trico_malloc(initial_buffer_size);
    if (!arch->buffer)
      {
      delete arch;
      return nullptr;
      }
    arch->buffer_pointer = arch->buffer;
    arch->buffer_size = initial_buffer_size;
    arch->size_available = initial_buffer_size;
    arch->writable = true;
    if (!write_header(arch))
      {
      delete arch;
      return nullptr;
      }
    return arch;
    }

  void* open_archive_for_reading(const uint8_t* data, uint64_t data_size)
    {
    archive* arch = new archive();
    arch->data = data;
    arch->data_pointer = arch->data;
    arch->data_size = data_size;
    if (!read_header(arch))
      {
      delete arch;
      return nullptr;
      }
    return arch;
    }

  void close_archive(void* a)
    {
    archive* arch = (archive*)a;
    if (arch->buffer)
      trico_free(arch->buffer);
    delete arch;
    }

  uint8_t* get_buffer_pointer(void* a)
    {
    archive* arch = (archive*)a;
    return arch->buffer;
    }

  uint64_t get_size(void* a)
    {
    archive* arch = (archive*)a;
    return arch->buffer_size - arch->size_available;
    }

  uint32_t get_version(void* a)
    {
    archive* arch = (archive*)a;
    return arch->version;
    }

  e_stream_type get_next_stream_type(void* a)
    {
    archive* arch = (archive*)a;
    return arch->next_stream_type;
    }

  void write_vertices(void* a, uint32_t nr_of_vertices, const float* vertices)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)vertex_float_stream;
    write(&header, 1, 1, arch);
    write(&nr_of_vertices, sizeof(uint32_t), 1, arch);

    float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    uint32_t nr_of_compressed_x_bytes;
    uint8_t* compressed_x;
    compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices);

    trico_free(x);
    write(&nr_of_compressed_x_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_x, 1, nr_of_compressed_x_bytes, arch);
    trico_free(compressed_x);

    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices);

    trico_free(y);
    write(&nr_of_compressed_y_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_y, 1, nr_of_compressed_y_bytes, arch);
    trico_free(compressed_y);

    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;
    compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices);

    trico_free(z);
    write(&nr_of_compressed_z_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_z, 1, nr_of_compressed_z_bytes, arch);
    trico_free(compressed_z);
    }


  void write_triangles(void* a, uint32_t nr_of_triangles, const uint32_t* tria_indices)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)triangle_uint32_stream;
    write(&header, 1, 1, arch);
    write(&nr_of_triangles, sizeof(uint32_t), 1, arch);

    uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);

    transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, tria_indices, nr_of_triangles * 3);


    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    delete[] compressed_buf;

    trico_free(b1);
    trico_free(b2);
    trico_free(b3);
    trico_free(b4);
    }

  void write_vertices(void* a, uint32_t nr_of_vertices, const double* vertices)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)vertex_double_stream;
    write(&header, 1, 1, arch);
    write(&nr_of_vertices, sizeof(uint32_t), 1, arch);

    double* x = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
    double* y = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
    double* z = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
    transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

    uint32_t nr_of_compressed_x_bytes;
    uint8_t* compressed_x;
    compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices);

    trico_free(x);
    write(&nr_of_compressed_x_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_x, 1, nr_of_compressed_x_bytes, arch);
    trico_free(compressed_x);

    uint32_t nr_of_compressed_y_bytes;
    uint8_t* compressed_y;
    compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices);

    trico_free(y);
    write(&nr_of_compressed_y_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_y, 1, nr_of_compressed_y_bytes, arch);
    trico_free(compressed_y);

    uint32_t nr_of_compressed_z_bytes;
    uint8_t* compressed_z;
    compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices);

    trico_free(z);
    write(&nr_of_compressed_z_bytes, sizeof(uint32_t), 1, arch);
    write(compressed_z, 1, nr_of_compressed_z_bytes, arch);
    trico_free(compressed_z);
    }

  void write_triangles(void* a, uint32_t nr_of_triangles, const uint64_t* tria_indices)
    {
    archive* arch = (archive*)a;
    uint8_t header = (uint8_t)triangle_uint64_stream;
    write(&header, 1, 1, arch);
    write(&nr_of_triangles, sizeof(uint32_t), 1, arch);

    uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b5 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b6 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b7 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint8_t* b8 = (uint8_t*)trico_malloc(nr_of_triangles * 3);

    transpose_uint64_aos_to_soa(&b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8, tria_indices, nr_of_triangles * 3);


    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    auto estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
    uint8_t* compressed_buf = new uint8_t[estimateLen];

    uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b5, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b6, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b7, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    bytes_written = (uint32_t)LZ4_compress_default((const char*)b8, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
    write(&bytes_written, sizeof(uint32_t), 1, arch);
    write(compressed_buf, 1, bytes_written, arch);

    delete[] compressed_buf;

    trico_free(b1);
    trico_free(b2);
    trico_free(b3);
    trico_free(b4);
    trico_free(b5);
    trico_free(b6);
    trico_free(b7);
    trico_free(b8);
    }

  uint32_t get_number_of_vertices(void* a)
    {
    archive* arch = (archive*)a;
    if (arch->next_stream_type == vertex_float_stream || arch->next_stream_type == vertex_double_stream)
      {
      uint32_t nr_vertices;
      read_inplace(&nr_vertices, sizeof(uint32_t), 1, arch);
      return nr_vertices;
      }
    return 0;
    }

  uint32_t get_number_of_triangles(void* a)
    {
    archive* arch = (archive*)a;
    if (arch->next_stream_type == triangle_uint32_stream || arch->next_stream_type == triangle_uint64_stream)
      {
      uint32_t nr_triangles;
      read_inplace(&nr_triangles, sizeof(uint32_t), 1, arch);
      return nr_triangles;
      }
    return 0;
    }

  bool read_vertices(void* a, float** vertices)
    {
    archive* arch = (archive*)a;
    if (get_next_stream_type(arch) != vertex_float_stream)
      return false;

    uint32_t nr_vertices;
    read(&nr_vertices, sizeof(uint32_t), 1, arch);

    uint32_t nr_of_compressed_bytes;
    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    void* compressed = trico_malloc(nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    float* decompressed_x;
    uint32_t nr_of_floats_x;
    decompress(&nr_of_floats_x, &decompressed_x, (const uint8_t*)compressed);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    float* decompressed_y;
    uint32_t nr_of_floats_y;
    decompress(&nr_of_floats_y, &decompressed_y, (const uint8_t*)compressed);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    float* decompressed_z;
    uint32_t nr_of_floats_z;
    decompress(&nr_of_floats_z, &decompressed_z, (const uint8_t*)compressed);
    trico_free(compressed);

    assert(nr_of_floats_x == nr_vertices);
    assert(nr_of_floats_x == nr_of_floats_y);
    assert(nr_of_floats_x == nr_of_floats_z);

    transpose_xyz_soa_to_aos(vertices, decompressed_x, decompressed_y, decompressed_z, nr_vertices);

    trico_free(decompressed_x);
    trico_free(decompressed_y);
    trico_free(decompressed_z);

    read_next_stream_type(arch);

    return true;
    }

  bool read_vertices(void* a, double** vertices)
    {
    archive* arch = (archive*)a;
   
    if (get_next_stream_type(arch) != vertex_double_stream)
      return false;

    uint32_t nr_vertices;
    read(&nr_vertices, sizeof(uint32_t), 1, arch);

    uint32_t nr_of_compressed_bytes;
    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    void* compressed = trico_malloc(nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    double* decompressed_x;
    uint32_t nr_of_doubles_x;
    decompress(&nr_of_doubles_x, &decompressed_x, (const uint8_t*)compressed);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    double* decompressed_y;
    uint32_t nr_of_doubles_y;
    decompress(&nr_of_doubles_y, &decompressed_y, (const uint8_t*)compressed);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    double* decompressed_z;
    uint32_t nr_of_doubles_z;
    decompress(&nr_of_doubles_z, &decompressed_z, (const uint8_t*)compressed);
    trico_free(compressed);

    assert(nr_of_doubles_x == nr_vertices);
    assert(nr_of_doubles_x == nr_of_doubles_y);
    assert(nr_of_doubles_x == nr_of_doubles_z);

    transpose_xyz_soa_to_aos(vertices, decompressed_x, decompressed_y, decompressed_z, nr_vertices);

    trico_free(decompressed_x);
    trico_free(decompressed_y);
    trico_free(decompressed_z);

    read_next_stream_type(arch);

    return true;
    }

  bool read_triangles(void* a, uint32_t** triangles)
    {
    archive* arch = (archive*)a;
    if (get_next_stream_type(arch) != triangle_uint32_stream)
      return false;

    uint32_t nr_of_triangles;
    read(&nr_of_triangles, sizeof(uint32_t), 1, arch);

    uint32_t nr_of_compressed_bytes;
    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    void* compressed = trico_malloc(nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    transpose_uint32_soa_to_aos(triangles, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, nr_of_triangles * 3);

    trico_free(compressed);
    trico_free(decompressed_b1);
    trico_free(decompressed_b2);
    trico_free(decompressed_b3);
    trico_free(decompressed_b4);

    read_next_stream_type(arch);

    return true;
    }

  bool read_triangles(void* a, uint64_t** triangles)
    {
    archive* arch = (archive*)a;
    
    if (get_next_stream_type(arch) != triangle_uint64_stream)
      return false;

    uint32_t nr_of_triangles;
    read(&nr_of_triangles, sizeof(uint32_t), 1, arch);

    uint32_t nr_of_compressed_bytes;
    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    void* compressed = trico_malloc(nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b5 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b5, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b6 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b6, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b7 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b7, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);

    read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch);
    compressed = trico_realloc(compressed, nr_of_compressed_bytes);
    read(compressed, 1, nr_of_compressed_bytes, arch);
    uint8_t* decompressed_b8 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
    bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b8, nr_of_compressed_bytes, nr_of_triangles * 3);
    assert(bytes_decompressed == nr_of_triangles * 3);


    transpose_uint64_soa_to_aos(triangles, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, decompressed_b5, decompressed_b6, decompressed_b7, decompressed_b8, nr_of_triangles * 3);

    trico_free(compressed);
    trico_free(decompressed_b1);
    trico_free(decompressed_b2);
    trico_free(decompressed_b3);
    trico_free(decompressed_b4);
    trico_free(decompressed_b5);
    trico_free(decompressed_b6);
    trico_free(decompressed_b7);
    trico_free(decompressed_b8);

    read_next_stream_type(arch);

    return true;
    }
  }