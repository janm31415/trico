#include <trico/alloc.h>
#include <trico/iostl.h>
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

int main(int argc, const char** argv)
  {
  if (argc < 2)
    {
    printf("Usage: stl_compress <filename.stl> [<output.trc> -n -a]\n");
    printf("  -n: include normals in archive\n");
    printf("  -a: include attributes in archive\n");
    return -1;
    }
  const char* filename = argv[1];
  char new_filename[1024];
  int include_normals = 0;
  int include_attributes = 0;
  int output_filename = 0;

  for (int j = 2; j < argc; ++j)
    {
    if (strcmp(argv[j], "-n") == 0)
      include_normals = 1;
    else if (strcmp(argv[j], "-a") == 0)
      include_attributes = 1;
    else
      {
      const char* ptr = argv[j];
      int idx = 0;
      while (*ptr)
        new_filename[idx++] = *ptr++;
      new_filename[idx] = 0;
      output_filename = 1;
      }
    }

  if (!output_filename)
    {
    change_extension_to_trc(new_filename, filename);
    }


  uint32_t nr_of_vertices;
  float* vertices = NULL;
  float* normals = NULL;
  uint32_t nr_of_triangles;
  uint32_t* triangles = NULL;
  uint16_t* attributes = NULL;

  int read_successfully = 0;
  if (include_normals || include_attributes)    
    read_successfully = trico_read_stl_full(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, &normals, &attributes, filename);    
  else    
    read_successfully = trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, filename);
    
  if (read_successfully != 1)
    {
    printf("Not a valid stl file: %s\n", filename);
    return -1;
    }

  void* arch = trico_open_archive_for_writing(1024 * 1024);
  if (!trico_write_vertices(arch, vertices, nr_of_vertices))
    {
    printf("Something went wrong when writing the vertices\n");
    return -1;
    }
  if (!trico_write_triangles(arch, triangles, nr_of_triangles))
    {
    printf("Something went wrong when writing the triangles\n");
    return -1;
    }
  if (include_normals)
    {
    if (!trico_write_normals(arch, normals, nr_of_triangles))
      {
      printf("Something went wrong when writing the normals\n");
      return -1;
      }
    }
  if (include_attributes)
    {
    if (!trico_write_attributes_uint16(arch, attributes, nr_of_triangles))
      {
      printf("Something went wrong when writing the attributes\n");
      return -1;
      }
    }

  trico_free(vertices);
  trico_free(triangles);
  trico_free(normals);
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