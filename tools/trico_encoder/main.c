#include <trico/alloc.h>
#include <trico_io/iostl.h>
#include <trico_io/ioply.h>
#include <trico/trico.h>

#include <stdio.h>
#include <string.h>

static void change_extension_to_trc(char* new_filename, const char* filename)
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
    new_filename[find_last_dot + 1] = 't';
    new_filename[find_last_dot + 2] = 'r';
    new_filename[find_last_dot + 3] = 'c';
    new_filename[find_last_dot + 4] = 0;
    }
  else
    {
    new_filename[filename_length + 0] = '.';
    new_filename[filename_length + 1] = 't';
    new_filename[filename_length + 2] = 'r';
    new_filename[filename_length + 3] = 'c';
    new_filename[filename_length + 4] = 0;
    }
  }

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

static void print_help()
  {
  printf("Usage: trico_encoder -i <input> [options]\n\n");
  printf("Options:\n");
  printf("  -i <input>           input file name of type binary stl or binary/ascii ply.\n");
  printf("  -o <output>          output file name.\n");
  printf("  -stladd <attribute>  add a given stl attribute (normal, uint16).\n");
  printf("  -plyskip <attribute> skip a given ply attribute (normal, tex_coord, color).\n");
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
  char new_filename[1024];
  int include_stl_normals = 0;
  int include_stl_uint16 = 0;
  int output_filename = 0;
  int skip_ply_normals = 0;
  int skip_ply_texcoords = 0;
  int skip_ply_color = 0;

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
    else if (strcmp(argv[j], "-stladd") == 0)
      {
      if (j == argc - 1)
        {
        printf("I expect an attribute after command -stladd\n");
        return -1;
        }
      ++j;
      if (strcmp(argv[j], "normal") == 0)
        {
        skip_ply_normals = 1;
        }
      else if (strcmp(argv[j], "tex_coord") == 0)
        {
        skip_ply_texcoords = 1;
        }
      else if (strcmp(argv[j], "color") == 0)
        {
        skip_ply_color = 1;
        }
      else
        {
        printf("Unknown attribute %s\n", argv[j]);
        return -1;
        }
      }
    else if (strcmp(argv[j], "-plyskip") == 0)
      {
      if (j == argc - 1)
        {
        printf("I expect an attribute after command -plyskip\n");
        return -1;
        }
      ++j;
      if (strcmp(argv[j], "normal") == 0)
        {
        include_stl_normals = 1;
        }
      else if (strcmp(argv[j], "uint16") == 0)
        {
        include_stl_uint16 = 1;
        }
      else
        {
        printf("Unknown attribute %s\n", argv[j]);
        return -1;
        }
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

  if (!output_filename)
    {
    change_extension_to_trc(new_filename, filename);
    }

  int is_stl = extension_is_stl(filename);
  int is_ply = extension_is_ply(filename);

  if (!is_stl && !is_ply)
    {
    printf("I expect the input file to be of type stl or ply.\n");
    return -1;
    }

  uint32_t nr_of_vertices = 0;
  float* vertices = NULL;
  float* triangle_normals = NULL;
  float* vertex_normals = NULL;
  uint32_t* vertex_colors = NULL;
  uint32_t nr_of_triangles = 0;
  uint32_t* triangles = NULL;
  float* texcoords = NULL;
  uint16_t* attributes = NULL;

  int read_successfully = 0;

  if (is_stl)
    {
    if (include_stl_normals || include_stl_uint16)
      read_successfully = trico_read_stl_full(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, &triangle_normals, &attributes, filename);
    else
      read_successfully = trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename);
    if (read_successfully != 1)
      {
      printf("Not a valid stl file: %s\n", filename);
      return -1;
      }
    }
  if (is_ply)
    {
    read_successfully = trico_read_ply(&nr_of_vertices, &vertices, &vertex_normals, &vertex_colors, &nr_of_triangles, &triangles, &texcoords, filename);
    if (read_successfully != 1)
      {
      printf("Not a valid ply file: %s\n", filename);
      return -1;
      }
    }  

  void* arch = trico_open_archive_for_writing(1024 * 1024);
  if (nr_of_vertices && vertices && !trico_write_vertices(arch, vertices, nr_of_vertices))
    {
    printf("Something went wrong when writing the vertices\n");
    return -1;
    }
  if (nr_of_triangles && triangles && !trico_write_triangles(arch, triangles, nr_of_triangles))
    {
    printf("Something went wrong when writing the triangles\n");
    return -1;
    }
  if (is_stl && include_stl_normals)
    {
    if (nr_of_triangles && triangle_normals && !trico_write_triangle_normals(arch, triangle_normals, nr_of_triangles))
      {
      printf("Something went wrong when writing the triangle normals\n");
      return -1;
      }
    }
  if (is_stl && include_stl_uint16)
    {
    if (nr_of_triangles && attributes && !trico_write_attributes_uint16(arch, attributes, nr_of_triangles))
      {
      printf("Something went wrong when writing the uint16 attributes\n");
      return -1;
      }
    }
  if (is_ply && !skip_ply_normals)
    {
    if (nr_of_vertices && vertex_normals && !trico_write_vertex_normals(arch, vertex_normals, nr_of_vertices))
      {
      printf("Something went wrong when writing the vertex normals\n");
      return -1;
      }      
    }
  if (is_ply && !skip_ply_color)
    {
    if (nr_of_vertices && vertex_colors && !trico_write_vertex_colors(arch, vertex_colors, nr_of_vertices))
      {
      printf("Something went wrong when writing the vertex colors\n");
      return -1;
      }
    }
  if (is_ply && !skip_ply_texcoords)
    {
    if (nr_of_triangles && texcoords && !trico_write_uv_per_triangle(arch, texcoords, nr_of_triangles))
      {
      printf("Something went wrong when writing the texture coordinates\n");
      return -1;
      }
    }

  trico_free(vertices);
  trico_free(vertex_normals);
  trico_free(vertex_colors);
  trico_free(triangles);
  trico_free(texcoords);
  trico_free(triangle_normals);
  trico_free(attributes);

  FILE* f = fopen(new_filename, "wb");
  if (!f)
    {
    printf("Cannot write to file %s\n", new_filename);
    return -1;
    }

  fwrite((const void*)trico_get_buffer_pointer(arch), trico_get_size(arch), 1, f);
  fclose(f);

  trico_close_archive(arch);

  return 0;
  }