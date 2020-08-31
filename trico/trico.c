#include "trico.h"
#include "transpose_aos_to_soa.h"
#include "floating_point_stream_compression.h"
#include "alloc.h"

#include <lz4/lz4.h>

#include <string.h>
#include <assert.h>


struct trico_archive
  {
  uint8_t* buffer;
  uint8_t* buffer_pointer;
  const uint8_t* data;
  const uint8_t* data_pointer;
  uint32_t version;
  enum trico_stream_type next_stream_type;
  uint64_t buffer_size;
  uint64_t data_size;
  uint64_t size_available;
  int writable;
  };

static int sufficient_buffer_available(struct trico_archive* arch, uint64_t bytes_needed)
  {
  return arch->size_available >= bytes_needed ? 1 : 0;
  }

static int buffer_ready_for_writing(struct trico_archive* arch, uint64_t bytes_needed)
  {
  if (!arch->writable)
    return 0;
  if (sufficient_buffer_available(arch, bytes_needed) == 0)
    {
    uint64_t extra_size = bytes_needed - arch->size_available;
    uint64_t buff_diff = arch->buffer_pointer - arch->buffer;
    arch->buffer = (uint8_t*)trico_realloc(arch->buffer, arch->buffer_size + extra_size);
    if (arch->buffer == NULL)
      return 0;
    arch->buffer_pointer = arch->buffer + buff_diff;
    arch->buffer_size += extra_size;
    arch->size_available += extra_size;
    }
  return 1;
  }

static void write_unsafe(const void* buf, uint64_t element_size, uint64_t element_count, struct trico_archive* arch)
  {
  uint64_t sz = element_size * element_count;
  memcpy(arch->buffer_pointer, buf, sz);
  arch->buffer_pointer += sz;
  arch->size_available -= sz;
  }

static int write(const void* buf, uint64_t element_size, uint64_t element_count, struct trico_archive* arch)
  {
  if (!buffer_ready_for_writing(arch, element_size * element_count))
    return 0;
  write_unsafe(buf, element_size, element_count, arch);
  return 1;
  }

static int read(void* buf, uint64_t element_size, uint64_t element_count, struct trico_archive* arch)
  {
  if (arch->writable)
    return 0;
  uint64_t sz = element_size * element_count;
  uint64_t data_read = arch->data_pointer - arch->data;
  if ((data_read + sz) > arch->data_size)
    return 0;
  memcpy(buf, arch->data_pointer, sz);
  arch->data_pointer += sz;
  return 1;
  }

static int read_inplace(void* buf, uint64_t element_size, uint64_t element_count, struct trico_archive* arch)
  {
  if (arch->writable)
    return 0;
  uint64_t sz = element_size * element_count;
  uint64_t data_read = arch->data_pointer - arch->data;
  if ((data_read + sz) > arch->data_size)
    return 0;
  memcpy(buf, arch->data_pointer, sz);
  return 1;
  }

static int write_header(struct trico_archive* arch)
  {
  if (buffer_ready_for_writing(arch, 8) == 0)
    return 0;
  uint32_t Trco = 0x6f637254;
  write_unsafe(&Trco, sizeof(uint32_t), 1, arch);
  write_unsafe(&(arch->version), sizeof(uint32_t), 1, arch);
  return 1;
  }

static void read_next_stream_type(struct trico_archive* arch)
  {
  assert(!arch->writable);
  if ((uint64_t)(arch->data_pointer - arch->data) < arch->data_size)
    {
    read(&(arch->next_stream_type), 1, 1, arch);
    }
  else
    arch->next_stream_type = empty;
  }

static int read_header(struct trico_archive* arch)
  {
  uint32_t Trco;
  if (!read(&Trco, sizeof(uint32_t), 1, arch))
    return 0;
  if (Trco != 0x6f637254) // not a trico file
    {
    return 0;
    }
  if (!read(&(arch->version), sizeof(uint32_t), 1, arch))
    return 0;
  read_next_stream_type(arch);
  return 1;
  }

void* trico_open_archive_for_writing(uint64_t initial_buffer_size)
  {
  struct trico_archive* arch = (struct trico_archive*)trico_malloc(sizeof(struct trico_archive));
  arch->buffer = NULL;
  arch->buffer_pointer = NULL;
  arch->data = NULL;
  arch->data_pointer = NULL;
  arch->version = 0;
  arch->next_stream_type = empty;
  arch->buffer_size = 0;
  arch->data_size = 0;
  arch->size_available = 0;
  arch->writable = 0;

  arch->buffer = (uint8_t*)trico_malloc(initial_buffer_size);
  if (!arch->buffer)
    {
    trico_free(arch);
    return NULL;
    }
  arch->buffer_pointer = arch->buffer;
  arch->buffer_size = initial_buffer_size;
  arch->size_available = initial_buffer_size;
  arch->writable = 1;
  if (write_header(arch) == 0)
    {
    trico_free(arch);
    return NULL;
    }
  return arch;
  }

void* trico_open_archive_for_reading(const uint8_t* data, uint64_t data_size)
  {
  struct trico_archive* arch = (struct trico_archive*)trico_malloc(sizeof(struct trico_archive));
  arch->buffer = NULL;
  arch->buffer_pointer = NULL;
  arch->data = NULL;
  arch->data_pointer = NULL;
  arch->version = 0;
  arch->next_stream_type = empty;
  arch->buffer_size = 0;
  arch->data_size = 0;
  arch->size_available = 0;
  arch->writable = 0;

  arch->data = data;
  arch->data_pointer = arch->data;
  arch->data_size = data_size;
  if (!read_header(arch))
    {
    trico_free(arch);
    return NULL;
    }
  return arch;
  }

void trico_close_archive(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->buffer)
    trico_free(arch->buffer);
  trico_free(arch);
  }

uint8_t* trico_get_buffer_pointer(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  return arch->buffer;
  }

uint64_t trico_get_size(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  return arch->buffer_size - arch->size_available;
  }

uint32_t trico_get_version(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  return arch->version;
  }

enum trico_stream_type trico_get_next_stream_type(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  return arch->next_stream_type;
  }

static int trico_write_vec3_float(void* a, const float* vertices, uint32_t nr_of_vertices, enum trico_stream_type st)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)st;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_vertices, sizeof(uint32_t), 1, arch))
    return 0;

  float* x = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* y = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  float* z = (float*)trico_malloc(sizeof(float)*nr_of_vertices);
  trico_transpose_xyz_aos_to_soa(&x, &y, &z, vertices, nr_of_vertices);

  uint32_t nr_of_compressed_x_bytes;
  uint8_t* compressed_x;
  trico_compress(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices, 4, 10);

  trico_free(x);
  if (!write(&nr_of_compressed_x_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_x, 1, nr_of_compressed_x_bytes, arch))
    return 0;
  trico_free(compressed_x);

  uint32_t nr_of_compressed_y_bytes;
  uint8_t* compressed_y;
  trico_compress(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices, 4, 10);

  trico_free(y);
  if (!write(&nr_of_compressed_y_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_y, 1, nr_of_compressed_y_bytes, arch))
    return 0;
  trico_free(compressed_y);

  uint32_t nr_of_compressed_z_bytes;
  uint8_t* compressed_z;
  trico_compress(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices, 4, 10);

  trico_free(z);
  if (!write(&nr_of_compressed_z_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_z, 1, nr_of_compressed_z_bytes, arch))
    return 0;
  trico_free(compressed_z);
  return 1;
  }

int trico_write_vertices(void* a, const float* vertices, uint32_t nr_of_vertices)
  {
  return trico_write_vec3_float(a, vertices, nr_of_vertices, vertex_float_stream);
  }

int trico_write_normals(void* a, const float* normals, uint32_t nr_of_normals)
  {
  return trico_write_vec3_float(a, normals, nr_of_normals, normal_float_stream);
  }

int trico_write_attributes_float(void* a, const float* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_float_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;  

  uint32_t nr_of_compressed_bytes;
  uint8_t* compressed;
  trico_compress(&nr_of_compressed_bytes, &compressed, attrib, nr_of_attribs, 4, 10);

  if (!write(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  trico_free(compressed);  

  return 1;
  }

int trico_write_attributes_double(void* a, const double* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_double_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  uint8_t* compressed;
  trico_compress_double_precision(&nr_of_compressed_bytes, &compressed, attrib, nr_of_attribs, 20, 20);

  if (!write(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  trico_free(compressed);

  return 1;
  }

int trico_write_triangles(void* a, const uint32_t* tria_indices, uint32_t nr_of_triangles)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)triangle_uint32_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_triangles, sizeof(uint32_t), 1, arch))
    return 0;

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);

  trico_transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, tria_indices, nr_of_triangles * 3);

  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  trico_free(b1);
  trico_free(b2);
  trico_free(b3);
  trico_free(b4);

  return 1;
  }

static int trico_write_vec3_double(void* a, const double* vertices, uint32_t nr_of_vertices, enum trico_stream_type st)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)st;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_vertices, sizeof(uint32_t), 1, arch))
    return 0;

  double* x = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
  double* y = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
  double* z = (double*)trico_malloc(sizeof(double)*nr_of_vertices);
  trico_transpose_xyz_aos_to_soa_double_precision(&x, &y, &z, vertices, nr_of_vertices);

  uint32_t nr_of_compressed_x_bytes;
  uint8_t* compressed_x;
  trico_compress_double_precision(&nr_of_compressed_x_bytes, &compressed_x, x, nr_of_vertices, 20, 20);

  trico_free(x);
  if (!write(&nr_of_compressed_x_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_x, 1, nr_of_compressed_x_bytes, arch))
    return 0;
  trico_free(compressed_x);

  uint32_t nr_of_compressed_y_bytes;
  uint8_t* compressed_y;
  trico_compress_double_precision(&nr_of_compressed_y_bytes, &compressed_y, y, nr_of_vertices, 20, 20);

  trico_free(y);
  if (!write(&nr_of_compressed_y_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_y, 1, nr_of_compressed_y_bytes, arch))
    return 0;
  trico_free(compressed_y);

  uint32_t nr_of_compressed_z_bytes;
  uint8_t* compressed_z;
  trico_compress_double_precision(&nr_of_compressed_z_bytes, &compressed_z, z, nr_of_vertices, 20, 20);

  trico_free(z);
  if (!write(&nr_of_compressed_z_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_z, 1, nr_of_compressed_z_bytes, arch))
    return 0;
  trico_free(compressed_z);
  return 1;
  }

int trico_write_vertices_double(void* a, const double* vertices, uint32_t nr_of_vertices)
  {
  return trico_write_vec3_double(a, vertices, nr_of_vertices, vertex_double_stream);
  }

int trico_write_normals_double(void* a, const double* normals, uint32_t nr_of_normals)
  {
  return trico_write_vec3_double(a, normals, nr_of_normals, normal_double_stream);
  }

int trico_write_triangles_long(void* a, const uint64_t* tria_indices, uint32_t nr_of_triangles)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)triangle_uint64_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_triangles, sizeof(uint32_t), 1, arch))
    return 0;

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b5 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b6 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b7 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint8_t* b8 = (uint8_t*)trico_malloc(nr_of_triangles * 3);

  trico_transpose_uint64_aos_to_soa(&b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8, tria_indices, nr_of_triangles * 3);


  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_triangles * 3);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b5, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b6, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b7, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b8, (char*)compressed_buf, (int)nr_of_triangles * 3, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  trico_free(b1);
  trico_free(b2);
  trico_free(b3);
  trico_free(b4);
  trico_free(b5);
  trico_free(b6);
  trico_free(b7);
  trico_free(b8);

  return 1;
  }

int trico_write_uv(void* a, const float* uv, uint32_t nr_of_uv_positions)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)uv_float_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_uv_positions, sizeof(uint32_t), 1, arch))
    return 0;

  float* u = (float*)trico_malloc(sizeof(float)*nr_of_uv_positions);
  float* v = (float*)trico_malloc(sizeof(float)*nr_of_uv_positions);
  trico_transpose_uv_aos_to_soa(&u, &v, uv, nr_of_uv_positions);

  uint32_t nr_of_compressed_u_bytes;
  uint8_t* compressed_u;
  trico_compress(&nr_of_compressed_u_bytes, &compressed_u, u, nr_of_uv_positions, 4, 10);

  trico_free(u);
  if (!write(&nr_of_compressed_u_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_u, 1, nr_of_compressed_u_bytes, arch))
    return 0;
  trico_free(compressed_u);

  uint32_t nr_of_compressed_v_bytes;
  uint8_t* compressed_v;
  trico_compress(&nr_of_compressed_v_bytes, &compressed_v, v, nr_of_uv_positions, 4, 10);

  trico_free(v);
  if (!write(&nr_of_compressed_v_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_v, 1, nr_of_compressed_v_bytes, arch))
    return 0;
  trico_free(compressed_v);

  return 1;
  }

int trico_write_uv_double(void* a, const double* uv, uint32_t nr_of_uv_positions)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)uv_double_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_uv_positions, sizeof(uint32_t), 1, arch))
    return 0;

  double* u = (double*)trico_malloc(sizeof(double)*nr_of_uv_positions);
  double* v = (double*)trico_malloc(sizeof(double)*nr_of_uv_positions);
  trico_transpose_uv_aos_to_soa_double_precision(&u, &v, uv, nr_of_uv_positions);

  uint32_t nr_of_compressed_u_bytes;
  uint8_t* compressed_u;
  trico_compress_double_precision(&nr_of_compressed_u_bytes, &compressed_u, u, nr_of_uv_positions, 20, 20);

  trico_free(u);
  if (!write(&nr_of_compressed_u_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_u, 1, nr_of_compressed_u_bytes, arch))
    return 0;
  trico_free(compressed_u);

  uint32_t nr_of_compressed_v_bytes;
  uint8_t* compressed_v;
  trico_compress_double_precision(&nr_of_compressed_v_bytes, &compressed_v, v, nr_of_uv_positions, 20, 20);

  trico_free(v);
  if (!write(&nr_of_compressed_v_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_v, 1, nr_of_compressed_v_bytes, arch))
    return 0;
  trico_free(compressed_v);

  return 1;
  }

int trico_write_attributes_uint8(void* a, const uint8_t* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_uint8_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0; 

  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_attribs);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)attrib, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  return 1;
  }

int trico_write_attributes_uint16(void* a, const uint16_t* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_uint16_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_attribs);

  trico_transpose_uint16_aos_to_soa(&b1, &b2, attrib, nr_of_attribs);

  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_attribs);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  trico_free(b1);
  trico_free(b2);

  return 1;
  }

int trico_write_attributes_uint32(void* a, const uint32_t* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_uint32_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_attribs);

  trico_transpose_uint32_aos_to_soa(&b1, &b2, &b3, &b4, attrib, nr_of_attribs);

  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_attribs);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  trico_free(b1);
  trico_free(b2);
  trico_free(b3);
  trico_free(b4);

  return 1;
  }

int trico_write_attributes_uint64(void* a, const uint64_t* attrib, uint32_t nr_of_attribs)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  uint8_t header = (uint8_t)attribute_uint64_stream;
  if (!write(&header, 1, 1, arch))
    return 0;
  if (!write(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint8_t* b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b2 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b3 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b4 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b5 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b6 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b7 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint8_t* b8 = (uint8_t*)trico_malloc(nr_of_attribs);

  trico_transpose_uint64_aos_to_soa(&b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8, attrib, nr_of_attribs);


  LZ4_stream_t lz4Stream_body;
  LZ4_stream_t* lz4Stream = &lz4Stream_body;
  LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

  unsigned estimateLen = LZ4_COMPRESSBOUND(nr_of_attribs);
  uint8_t* compressed_buf = (uint8_t*)trico_malloc(estimateLen);

  uint32_t bytes_written = (uint32_t)LZ4_compress_default((const char*)b1, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b2, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b3, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b4, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b5, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b6, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b7, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  bytes_written = (uint32_t)LZ4_compress_default((const char*)b8, (char*)compressed_buf, (int)nr_of_attribs, (int)estimateLen);
  if (!write(&bytes_written, sizeof(uint32_t), 1, arch))
    return 0;
  if (!write(compressed_buf, 1, bytes_written, arch))
    return 0;

  trico_free(compressed_buf);

  trico_free(b1);
  trico_free(b2);
  trico_free(b3);
  trico_free(b4);
  trico_free(b5);
  trico_free(b6);
  trico_free(b7);
  trico_free(b8);

  return 1;
  }

uint32_t trico_get_number_of_vertices(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->next_stream_type == vertex_float_stream || arch->next_stream_type == vertex_double_stream)
    {
    uint32_t nr_vertices;
    if (!read_inplace(&nr_vertices, sizeof(uint32_t), 1, arch))
      return 0;
    return nr_vertices;
    }
  return 0;
  }

uint32_t trico_get_number_of_triangles(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->next_stream_type == triangle_uint32_stream || arch->next_stream_type == triangle_uint64_stream)
    {
    uint32_t nr_triangles;
    if (!read_inplace(&nr_triangles, sizeof(uint32_t), 1, arch))
      return 0;
    return nr_triangles;
    }
  return 0;
  }

uint32_t trico_get_number_of_uvs(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->next_stream_type == uv_float_stream || arch->next_stream_type == uv_double_stream)
    {
    uint32_t nr_uvs;
    if (!read_inplace(&nr_uvs, sizeof(uint32_t), 1, arch))
      return 0;
    return nr_uvs;
    }
  return 0;
  }

uint32_t trico_get_number_of_normals(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->next_stream_type == normal_float_stream || arch->next_stream_type == normal_double_stream)
    {
    uint32_t nr_normals;
    if (!read_inplace(&nr_normals, sizeof(uint32_t), 1, arch))
      return 0;
    return nr_normals;
    }
  return 0;
  }

uint32_t trico_get_number_of_attributes(void* a)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (arch->next_stream_type == attribute_float_stream || arch->next_stream_type == attribute_double_stream ||
    arch->next_stream_type == attribute_uint8_stream || arch->next_stream_type == attribute_uint16_stream ||
    arch->next_stream_type == attribute_uint32_stream || arch->next_stream_type == attribute_uint64_stream)
    {
    uint32_t nr_attribs;
    if (!read_inplace(&nr_attribs, sizeof(uint32_t), 1, arch))
      return 0;
    return nr_attribs;
    }
  return 0;
  }

static int trico_read_vec3_float(void* a, float** vertices, enum trico_stream_type st)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != st)
    return 0;

  uint32_t nr_vertices;
  if (!read(&nr_vertices, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  float* decompressed_x;
  uint32_t nr_of_floats_x;
  trico_decompress(&nr_of_floats_x, &decompressed_x, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  float* decompressed_y;
  uint32_t nr_of_floats_y;
  trico_decompress(&nr_of_floats_y, &decompressed_y, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  float* decompressed_z;
  uint32_t nr_of_floats_z;
  trico_decompress(&nr_of_floats_z, &decompressed_z, (const uint8_t*)compressed);
  trico_free(compressed);

  assert(nr_of_floats_x == nr_vertices);
  assert(nr_of_floats_x == nr_of_floats_y);
  assert(nr_of_floats_x == nr_of_floats_z);

  trico_transpose_xyz_soa_to_aos(vertices, decompressed_x, decompressed_y, decompressed_z, nr_vertices);

  trico_free(decompressed_x);
  trico_free(decompressed_y);
  trico_free(decompressed_z);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_vertices(void* a, float** vertices)
  {
  return trico_read_vec3_float(a, vertices, vertex_float_stream);
  }

int trico_read_normals(void* a, float** normals)
  {
  return trico_read_vec3_float(a, normals, normal_float_stream);
  }

static int trico_read_vec3_double(void* a, double** vertices, enum trico_stream_type st)
  {
  struct trico_archive* arch = (struct trico_archive*)a;

  if (trico_get_next_stream_type(arch) != st)
    return 0;

  uint32_t nr_vertices;
  if (!read(&nr_vertices, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  double* decompressed_x;
  uint32_t nr_of_doubles_x;
  trico_decompress_double_precision(&nr_of_doubles_x, &decompressed_x, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  double* decompressed_y;
  uint32_t nr_of_doubles_y;
  trico_decompress_double_precision(&nr_of_doubles_y, &decompressed_y, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  double* decompressed_z;
  uint32_t nr_of_doubles_z;
  trico_decompress_double_precision(&nr_of_doubles_z, &decompressed_z, (const uint8_t*)compressed);
  trico_free(compressed);

  assert(nr_of_doubles_x == nr_vertices);
  assert(nr_of_doubles_x == nr_of_doubles_y);
  assert(nr_of_doubles_x == nr_of_doubles_z);

  trico_transpose_xyz_soa_to_aos_double_precision(vertices, decompressed_x, decompressed_y, decompressed_z, nr_vertices);

  trico_free(decompressed_x);
  trico_free(decompressed_y);
  trico_free(decompressed_z);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_vertices_double(void* a, double** vertices)
  {
  return trico_read_vec3_double(a, vertices, vertex_double_stream);
  }

int trico_read_normals_double(void* a, double** normals)
  {
  return trico_read_vec3_double(a, normals, normal_double_stream);
  }

int trico_read_triangles(void* a, uint32_t** triangles)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != triangle_uint32_stream)
    return 0;

  uint32_t nr_of_triangles;
  if (!read(&nr_of_triangles, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  trico_transpose_uint32_soa_to_aos(triangles, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, nr_of_triangles * 3);

  trico_free(compressed);
  trico_free(decompressed_b1);
  trico_free(decompressed_b2);
  trico_free(decompressed_b3);
  trico_free(decompressed_b4);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_triangles_long(void* a, uint64_t** triangles)
  {
  struct trico_archive* arch = (struct trico_archive*)a;

  if (trico_get_next_stream_type(arch) != triangle_uint64_stream)
    return 0;

  uint32_t nr_of_triangles;
  if (!read(&nr_of_triangles, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b5 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b5, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b6 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b6, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b7 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b7, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b8 = (uint8_t*)trico_malloc(nr_of_triangles * 3);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b8, nr_of_compressed_bytes, nr_of_triangles * 3);
  assert(bytes_decompressed == nr_of_triangles * 3);


  trico_transpose_uint64_soa_to_aos(triangles, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, decompressed_b5, decompressed_b6, decompressed_b7, decompressed_b8, nr_of_triangles * 3);

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

  return 1;
  }

int trico_read_uv(void* a, float** uv)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != uv_float_stream)
    return 0;

  uint32_t nr_uv_positions;
  if (!read(&nr_uv_positions, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  float* decompressed_u;
  uint32_t nr_of_floats_u;
  trico_decompress(&nr_of_floats_u, &decompressed_u, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  float* decompressed_v;
  uint32_t nr_of_floats_v;
  trico_decompress(&nr_of_floats_v, &decompressed_v, (const uint8_t*)compressed);

  trico_free(compressed);

  assert(nr_of_floats_u == nr_uv_positions);
  assert(nr_of_floats_v == nr_of_floats_u);

  trico_transpose_uv_soa_to_aos(uv, decompressed_u, decompressed_v, nr_uv_positions);

  trico_free(decompressed_u);
  trico_free(decompressed_v);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_uv_double(void* a, double** uv)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != uv_double_stream)
    return 0;

  uint32_t nr_uv_positions;
  if (!read(&nr_uv_positions, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  double* decompressed_u;
  uint32_t nr_of_doubles_u;
  trico_decompress_double_precision(&nr_of_doubles_u, &decompressed_u, (const uint8_t*)compressed);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  double* decompressed_v;
  uint32_t nr_of_doubles_v;
  trico_decompress_double_precision(&nr_of_doubles_v, &decompressed_v, (const uint8_t*)compressed);

  trico_free(compressed);

  assert(nr_of_doubles_u == nr_uv_positions);
  assert(nr_of_doubles_v == nr_of_doubles_u);

  trico_transpose_uv_soa_to_aos_double_precision(uv, decompressed_u, decompressed_v, nr_uv_positions);

  trico_free(decompressed_u);
  trico_free(decompressed_v);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_float(void* a, float** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != attribute_float_stream)
    return 0;

  uint32_t nr_attrib;
  if (!read(&nr_attrib, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint32_t nr_of_floats;
  trico_decompress(&nr_of_floats, attrib, (const uint8_t*)compressed);

  trico_free(compressed);
  assert(nr_of_floats == nr_attrib);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_double(void* a, double** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != attribute_double_stream)
    return 0;

  uint32_t nr_attrib;
  if (!read(&nr_attrib, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint32_t nr_of_doubles;
  trico_decompress_double_precision(&nr_of_doubles, attrib, (const uint8_t*)compressed);

  trico_free(compressed);
  assert(nr_of_doubles == nr_attrib);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_uint8(void* a, uint8_t** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != attribute_uint8_stream)
    return 0;

  uint32_t nr_of_attribs;
  if (!read(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)attrib, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  trico_free(compressed);  

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_uint16(void* a, uint16_t** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != attribute_uint16_stream)
    return 0;

  uint32_t nr_of_attribs;
  if (!read(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  trico_transpose_uint16_soa_to_aos(attrib, decompressed_b1, decompressed_b2, nr_of_attribs);

  trico_free(compressed);
  trico_free(decompressed_b1);
  trico_free(decompressed_b2);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_uint32(void* a, uint32_t** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;
  if (trico_get_next_stream_type(arch) != attribute_uint32_stream)
    return 0;

  uint32_t nr_of_attribs;
  if (!read(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  trico_transpose_uint32_soa_to_aos(attrib, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, nr_of_attribs);

  trico_free(compressed);
  trico_free(decompressed_b1);
  trico_free(decompressed_b2);
  trico_free(decompressed_b3);
  trico_free(decompressed_b4);

  read_next_stream_type(arch);

  return 1;
  }

int trico_read_attributes_uint64(void* a, uint64_t** attrib)
  {
  struct trico_archive* arch = (struct trico_archive*)a;

  if (trico_get_next_stream_type(arch) != attribute_uint64_stream)
    return 0;

  uint32_t nr_of_attribs;
  if (!read(&nr_of_attribs, sizeof(uint32_t), 1, arch))
    return 0;

  uint32_t nr_of_compressed_bytes;
  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  void* compressed = trico_malloc(nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b1 = (uint8_t*)trico_malloc(nr_of_attribs);
  uint32_t bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b1, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b2 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b2, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b3 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b3, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b4 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b4, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b5 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b5, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b6 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b6, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b7 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b7, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);

  if (!read(&nr_of_compressed_bytes, sizeof(uint32_t), 1, arch))
    return 0;
  compressed = trico_realloc(compressed, nr_of_compressed_bytes);
  if (!read(compressed, 1, nr_of_compressed_bytes, arch))
    return 0;
  uint8_t* decompressed_b8 = (uint8_t*)trico_malloc(nr_of_attribs);
  bytes_decompressed = (uint32_t)LZ4_decompress_safe((const char*)compressed, (char*)decompressed_b8, nr_of_compressed_bytes, nr_of_attribs);
  assert(bytes_decompressed == nr_of_attribs);


  trico_transpose_uint64_soa_to_aos(attrib, decompressed_b1, decompressed_b2, decompressed_b3, decompressed_b4, decompressed_b5, decompressed_b6, decompressed_b7, decompressed_b8, nr_of_attribs);

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

  return 1;
  }