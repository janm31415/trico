#include <trico/alloc.h>
#include <trico_io/ioply.h>
#include <trico_io/iostl.h>
#include <trico/trico.h>

#include <stdio.h>
#include <string.h>
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

static int extension_is_stl(const char* filename)
  {
  const char* filename_ptr = filename;
  int filename_length = 0;
  while (*filename_ptr++)
    ++filename_length;

  int find_last_dot = filename_length - 1;
  while (find_last_dot)
    {
    if (filename[find_last_dot] == '.')
      break;
    --find_last_dot;
    }
  if (filename[find_last_dot] != '.')
    return 0;
  if ((filename_length - find_last_dot) != 4)
    return 0;
  if ((filename[find_last_dot + 1] == 's' || filename[find_last_dot + 1] == 'S') &&
    (filename[find_last_dot + 2] == 't' || filename[find_last_dot + 2] == 'T') &&
    (filename[find_last_dot + 3] == 'l' || filename[find_last_dot + 3] == 'L'))
    return 1;
  return 0;
  }

static int extension_is_ply(const char* filename)
  {
  const char* filename_ptr = filename;
  int filename_length = 0;
  while (*filename_ptr++)
    ++filename_length;

  int find_last_dot = filename_length - 1;
  while (find_last_dot)
    {
    if (filename[find_last_dot] == '.')
      break;
    --find_last_dot;
    }
  if (filename[find_last_dot] != '.')
    return 0;
  if ((filename_length - find_last_dot) != 4)
    return 0;
  if ((filename[find_last_dot + 1] == 'p' || filename[find_last_dot + 1] == 'P') &&
    (filename[find_last_dot + 2] == 'l' || filename[find_last_dot + 2] == 'L') &&
    (filename[find_last_dot + 3] == 'y' || filename[find_last_dot + 3] == 'Y'))
    return 1;
  return 0;
  }

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

static void change_extension_to_ply(char* new_filename, const char* filename)
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
    new_filename[find_last_dot + 1] = 'p';
    new_filename[find_last_dot + 2] = 'l';
    new_filename[find_last_dot + 3] = 'y';
    new_filename[find_last_dot + 4] = 0;
    }
  else
    {
    new_filename[filename_length + 0] = '.';
    new_filename[filename_length + 1] = 'p';
    new_filename[filename_length + 2] = 'l';
    new_filename[filename_length + 3] = 'y';
    new_filename[filename_length + 4] = 0;
    }
  }

static void print_help()
  {
  printf("Usage: trico_decoder -i <input> [options]\n\n");
  printf("Options:\n");
  printf("  -i <input>           input file name.\n");
  printf("  -o <output>          output file name of type stl or ply.\n");
  printf("\n");
  }

int main(int argc, const char** argv)
  {
  if (argc < 3)
    {
    print_help();
    return -1;
    }
  const char* filename = NULL;
  int output_filename = 0;
  char new_filename[1024];
  for (int j = 1; j < argc; ++j)
    {
    if (strcmp(argv[j], "-i") == 0)
      {
      if (j == argc - 1)
        {
        printf("I expect a filename after command -i\n");
        return -1;
        }
      ++j;
      filename = argv[j];
      }
    else if (strcmp(argv[j], "-o") == 0)
      {
      if (j == argc - 1)
        {
        printf("I expect a filename after command -o\n");
        return -1;
        }
      ++j;
      const char* ptr = argv[j];
      int idx = 0;
      while (*ptr)
        new_filename[idx++] = *ptr++;
      new_filename[idx] = 0;
      output_filename = 1;
      }
    else
      {
      printf("Unknown command %s\n", argv[j]);
      return -1;
      }
    }

  if (!filename)
    {
    printf("An input file name is required\n");
    return -1;
    }


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
  float* triangle_normals = NULL;
  float* vertex_normals = NULL;
  uint32_t* vertex_colors = NULL;
  float* texcoords = NULL;
  uint16_t* attributes = NULL;
  uint32_t nr_of_vertices = 0;
  uint32_t nr_of_triangles = 0;
  uint32_t nr_of_triangle_normals = 0;
  uint32_t nr_of_vertex_normals = 0;
  uint32_t nr_of_vertex_colors = 0;
  uint32_t nr_of_texcoords = 0;
  uint32_t nr_of_attributes = 0;

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
      case trico_triangle_normal_float_stream:
      {
      nr_of_triangle_normals = trico_get_number_of_normals(arch);
      triangle_normals = (float*)malloc(nr_of_triangle_normals * 3 * sizeof(float));
      if (!trico_read_triangle_normals(arch, &triangle_normals))
        {
        free(triangle_normals);
        printf("Something went wrong when reading the triangle normals\n");
        return -1;
        }
      break;
      }
      case trico_vertex_normal_float_stream:
      {
      nr_of_vertex_normals = trico_get_number_of_normals(arch);
      vertex_normals = (float*)malloc(nr_of_vertex_normals * 3 * sizeof(float));
      if (!trico_read_vertex_normals(arch, &vertex_normals))
        {
        free(vertex_normals);
        printf("Something went wrong when reading the vertex normals\n");
        return -1;
        }
      break;
      }
      case trico_vertex_color_stream:
      {
      nr_of_vertex_colors = trico_get_number_of_colors(arch);
      vertex_colors = (uint32_t*)malloc(nr_of_vertex_colors * sizeof(uint32_t));
      if (!trico_read_vertex_colors(arch, &vertex_colors))
        {
        free(vertex_colors);
        printf("Something went wrong when reading the vertex colors\n");
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
      case trico_uv_per_triangle_float_stream:
      {
      nr_of_texcoords = trico_get_number_of_uvs(arch);
      texcoords = (float*)malloc(nr_of_texcoords * 2 * sizeof(float));
      if (!trico_read_uv_per_triangle(arch, &texcoords))
        {
        free(texcoords);
        printf("Something went wrong when reading the texture coordinates\n");
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

  int output_as_stl = 0;
  int output_as_ply = 0;

  if (output_filename)
    {
    output_as_stl = extension_is_stl(new_filename);
    output_as_ply = extension_is_ply(new_filename);
    }

  if (!output_as_stl && !output_as_ply)
    {
    if (vertex_colors || texcoords || vertex_normals)
      output_as_ply = 1;
    else
      output_as_stl = 1;
    }

  if (!output_filename)
    {
    if (output_as_ply)
      change_extension_to_ply(new_filename, filename);
    else
      change_extension_to_stl(new_filename, filename);
    }

  if (output_as_stl && (triangle_normals == NULL))
    {
    triangle_normals = (float*)malloc(nr_of_triangles * 3 * sizeof(float));
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
      triangle_normals[t * 3] = length ? nx / length : nx;
      triangle_normals[t * 3 + 1] = length ? ny / length : ny;
      triangle_normals[t * 3 + 2] = length ? nz / length : nz;
      }
    }

  if (output_as_stl)
    {
    if (!trico_write_stl(vertices, tria_indices, nr_of_triangles, triangle_normals, attributes, new_filename))
      {
      printf("Could not write to %s\n", new_filename);
      return -1;
      }
    }
  else
    {
    if (!trico_write_ply(nr_of_vertices, vertices, vertex_normals, vertex_colors, nr_of_triangles, tria_indices, texcoords, new_filename))
      {
      printf("Could not write to %s\n", new_filename);
      return -1;
      }
    }
  
  free(vertices);
  free(triangle_normals);
  free(vertex_normals);
  free(vertex_colors);
  free(texcoords);
  free(tria_indices);
  free(attributes);
  return 0;
  }