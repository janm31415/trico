#include <trico/alloc.h>
#include <trico/iostl.h>
#include <trico/trico.h>

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

#ifdef _WIN32
static inline long long fsize(const char *filename)
  {
  struct _stat64 st;

  if (_stat64(filename, &st) == 0)
    return st.st_size;

  return -1;
  }
#else
static inline long long fsize(const char *filename) 
  {
  struct stat st;

  if (stat(filename, &st) == 0)
    return st.st_size;

  return -1;
  }
#endif

static void change_extension_to_stl(char* new_filename, const char* filename)
  {
  const char* filename_ptr = filename;
  int filename_length = 0;
  while (*filename_ptr)
    new_filename[filename_length++] = *filename_ptr++;

  int find_last_dot = filename_length - 1;
  while (find_last_dot)
    {
    if (new_filename[find_last_dot] == '.')
      break;
    --find_last_dot;
    }
  if (new_filename[find_last_dot] == '.')
    {
    new_filename[find_last_dot + 1] = 's';
    new_filename[find_last_dot + 2] = 't';
    new_filename[find_last_dot + 3] = 'l';
    new_filename[find_last_dot + 4] = 0;
    }
  else
    {
    new_filename[filename_length + 0] = '.';
    new_filename[filename_length + 1] = 's';
    new_filename[filename_length + 2] = 't';
    new_filename[filename_length + 3] = 'l';
    new_filename[filename_length + 4] = 0;
    }
  }

int main(int argc, const char** argv)
  {
  if (argc < 2)
    {
    printf("Usage: stl_decompress <filename.trc> [<output.stl>]\n");
    return -1;
    }
  const char* filename = argv[1];

  long long size = fsize(filename);
  if (size < 0)
    {
    printf("There was an error reading file %s\n", filename);
    return -1;
    }

  FILE* f = fopen(filename, "rb");
  if (!f)
    {
    printf("Cannot open file: %s\n", filename);
    return -1;
    }  
  char* buffer = (char*)malloc(size); 
  long long fl = (long long)fread(buffer, 1, size, f);
  if (fl != size)
    {
    printf("There was an error reading file %s\n", filename);
    return -1;
    }
  fclose(f);

  void* arch = trico_open_archive_for_reading((const uint8_t*)buffer, size);
  if (!arch)
    {
    printf("The input file %s is not a trico archive.\n", filename);
    return -1;
    }

  float* vertices = NULL;
  uint32_t* tria_indices = NULL;
  uint32_t nr_of_vertices = 0;
  uint32_t nr_of_triangles = 0;
  uint32_t nr_of_normals = 0;
  uint32_t nr_of_attributes = 0;
  float* normals = NULL;
  uint16_t* attributes = NULL;

  enum trico_stream_type st = trico_get_next_stream_type(arch);
  while (!st == trico_empty)
    {
    switch (st)
      {
      case trico_vertex_float_stream:
      {
      nr_of_vertices = trico_get_number_of_vertices(arch);
      vertices = (float*)malloc(nr_of_vertices * 3 * sizeof(float));
      if (!trico_read_vertices(arch, &vertices))
        {
        free(vertices);
        printf("Something went wrong when reading the vertices\n");
        return -1;
        }
      break;
      }
      case trico_normal_float_stream:
      {
      nr_of_normals = trico_get_number_of_normals(arch);
      normals = (float*)malloc(nr_of_normals * 3 * sizeof(float));
      if (!trico_read_normals(arch, &normals))
        {
        free(normals);
        printf("Something went wrong when reading the normals\n");
        return -1;
        }
      break;
      }
      case trico_triangle_uint32_stream:
      {
      nr_of_triangles = trico_get_number_of_triangles(arch);
      tria_indices = (uint32_t*)malloc(nr_of_triangles * 3 * sizeof(uint32_t));
      if (!trico_read_triangles(arch, &tria_indices))
        {
        free(tria_indices);
        printf("Something went wrong when reading the triangles\n");
        return -1;
        }
      break;
      }
      case trico_attribute_uint16_stream:
      {
      nr_of_attributes = trico_get_number_of_attributes(arch);
      attributes = (uint16_t*)malloc(nr_of_attributes * sizeof(uint16_t));
      if (!trico_read_attributes_uint16(arch, &attributes))
        {
        free(attributes);
        printf("Something went wrong when reading the attributes\n");
        return -1;
        }
      break;
      }
      default:
      {
      trico_skip_next_stream(arch);
      break;
      }
      }

    st = trico_get_next_stream_type(arch);
    }

  trico_close_archive(arch);
  free(buffer);  

  if (normals == NULL)
    {
    normals = (float*)malloc(nr_of_triangles * 3 * sizeof(float));
    for (uint32_t t = 0; t < nr_of_triangles; ++t)
      {
      const uint32_t v0 = tria_indices[t * 3];
      const uint32_t v1 = tria_indices[t * 3 + 1];
      const uint32_t v2 = tria_indices[t * 3 + 2];
      const float x0 = vertices[v0 * 3];
      const float y0 = vertices[v0 * 3 + 1];
      const float z0 = vertices[v0 * 3 + 2];
      const float x1 = vertices[v1 * 3];
      const float y1 = vertices[v1 * 3 + 1];
      const float z1 = vertices[v1 * 3 + 2];
      const float x2 = vertices[v2 * 3];
      const float y2 = vertices[v2 * 3 + 1];
      const float z2 = vertices[v2 * 3 + 2];
      const float ax = x1 - x0;
      const float ay = y1 - y0;
      const float az = z1 - z0;
      const float bx = x2 - x0;
      const float by = y2 - y0;
      const float bz = z2 - z0;      
      const float nx = ay * bz - az * by;
      const float ny = az * bx - ax * bz;
      const float nz = ax * by - ay * bx;
      const float length = (float)sqrt((double)(nx*nx + ny * ny + nz * nz));
      normals[t * 3] = length ? nx / length : nx;
      normals[t * 3 + 1] = length ? ny / length : ny;
      normals[t * 3 + 2] = length ? nz / length : nz;
      }
    }

  if (argc == 2)
    {
    char new_filename[1024];
    change_extension_to_stl(new_filename, filename);

    if (!trico_write_stl(vertices, tria_indices, nr_of_triangles, normals, attributes, new_filename))
      {
      printf("Could not write to %s\n", new_filename);
      return -1;
      }
    }
  else
    {
    if (!trico_write_stl(vertices, tria_indices, nr_of_triangles, normals, attributes, argv[2]))
      {
      printf("Could not write to %s\n", argv[2]);
      return -1;
      }
    }

  free(vertices);
  free(normals);
  free(tria_indices);
  free(attributes);
  return 0;
  }