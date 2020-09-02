#include "ioply.h"
#include "alloc.h"
#include <stdio.h>
#include <string.h>

#include <rply/rply.h>


static int read_vec3_coord(p_ply_argument argument)
  {
  float** p_vertices;
  ply_get_argument_user_data(argument, (void**)&p_vertices, NULL);
  double val = ply_get_argument_value(argument);
  *(*p_vertices) = (float)val;
  (*p_vertices) += 3;
  return 1;
  }

static int read_color(p_ply_argument argument)
  {
  uint8_t** p_color;
  ply_get_argument_user_data(argument, (void**)&p_color, NULL);
  double val = ply_get_argument_value(argument);
  *(*p_color) = (uint8_t)val;
  (*p_color) += 4;
  return 1;
  }

static int read_face(p_ply_argument argument)
  {
  uint32_t** p_triangle;
  ply_get_argument_user_data(argument, (void**)&p_triangle, NULL);
  long length, value_index;
  ply_get_argument_property(argument, NULL, &length, &value_index);
  if (value_index >= 0 && value_index < 3)
    {
    double val = ply_get_argument_value(argument);
    *(*p_triangle) = (uint32_t)val;
    ++(*p_triangle);
    }
  return 1;
  }

static int read_texcoord(p_ply_argument argument)
  {
  float** p_uv;
  ply_get_argument_user_data(argument, (void**)&p_uv, NULL);
  long length, value_index;
  ply_get_argument_property(argument, NULL, &length, &value_index);
  if (value_index >= 0 && value_index < 6)
    {
    double val = ply_get_argument_value(argument);
    *(*p_uv) = (float)val;
    ++(*p_uv);
    }
  if (value_index == (length - 1) && (length != 6)) // this case happens when the ply file has less than 6 texture coordinates for a face
    {
    for (long j = length; j < 6; ++j) // fill the remaining texture coordinates with 0.f
      {
      *(*p_uv) = (float)0.f;
      ++(*p_uv);
      }
    }
  return 1;
  }

int trico_read_ply(uint32_t* nr_of_vertices,
  float** vertices,
  float** vertex_normals,
  uint32_t** vertex_colors,
  uint32_t* nr_of_triangles,
  uint32_t** triangles,
  float** texcoords,
  const char* filename)
  {
  *nr_of_vertices = 0;
  *vertices = NULL;
  *vertex_normals = NULL;
  *vertex_colors = NULL;
  *nr_of_triangles = 0;
  *triangles = NULL;
  *texcoords = NULL;

  p_ply ply = ply_open(filename, NULL, 0, NULL);
  if (!ply)
    return 0;
  if (!ply_read_header(ply))
    return 0;

  float* p_vertex_pointer_x = NULL;
  float* p_vertex_pointer_y = NULL;
  float* p_vertex_pointer_z = NULL;

  long nvertices_x = ply_set_read_cb(ply, "vertex", "x", read_vec3_coord, (void*)(&p_vertex_pointer_x), 0);
  long nvertices_y = ply_set_read_cb(ply, "vertex", "y", read_vec3_coord, (void*)(&p_vertex_pointer_y), 0);
  long nvertices_z = ply_set_read_cb(ply, "vertex", "z", read_vec3_coord, (void*)(&p_vertex_pointer_z), 0);

  if ((nvertices_x != nvertices_y) || (nvertices_x != nvertices_z))
    {
    ply_close(ply);
    return 0;
    }

  *nr_of_vertices = (uint32_t)nvertices_x;

  if (nvertices_x > 0)
    *vertices = (float*)trico_malloc(*nr_of_vertices * 3 * sizeof(float));
  p_vertex_pointer_x = (float*)(*vertices);
  p_vertex_pointer_y = p_vertex_pointer_x + 1;
  p_vertex_pointer_z = p_vertex_pointer_x + 2;

  float* p_normal_pointer_x = NULL;
  float* p_normal_pointer_y = NULL;
  float* p_normal_pointer_z = NULL;

  long nnormals_x = ply_set_read_cb(ply, "vertex", "nx", read_vec3_coord, (void*)(&p_normal_pointer_x), 0);
  long nnormals_y = ply_set_read_cb(ply, "vertex", "ny", read_vec3_coord, (void*)(&p_normal_pointer_y), 0);
  long nnormals_z = ply_set_read_cb(ply, "vertex", "nz", read_vec3_coord, (void*)(&p_normal_pointer_z), 0);

  if ((nnormals_x != nnormals_y) || (nnormals_x != nnormals_z) || (nnormals_x && ((uint32_t)nnormals_x != *nr_of_vertices)))
    {
    if (*nr_of_vertices)
      {
      free(*vertices);
      *vertices = NULL;
      *nr_of_vertices = 0;
      }
    ply_close(ply);
    return 0;
    }

  if (nnormals_x > 0)
    *vertex_normals = (float*)trico_malloc(*nr_of_vertices * 3 * sizeof(float));
  p_normal_pointer_x = (float*)(*vertex_normals);
  p_normal_pointer_y = p_normal_pointer_x + 1;
  p_normal_pointer_z = p_normal_pointer_x + 2;

  uint8_t* p_red = NULL;
  uint8_t* p_green = NULL;
  uint8_t* p_blue = NULL;
  uint8_t* p_alpha = NULL;

  long nred = ply_set_read_cb(ply, "vertex", "red", read_color, (void*)(&p_red), 0);
  long ngreen = ply_set_read_cb(ply, "vertex", "green", read_color, (void*)(&p_green), 0);
  long nblue = ply_set_read_cb(ply, "vertex", "blue", read_color, (void*)(&p_blue), 0);
  long nalpha = ply_set_read_cb(ply, "vertex", "alpha", read_color, (void*)(&p_alpha), 0);

  if (nred == 0)
    nred = ply_set_read_cb(ply, "vertex", "r", read_color, (void*)(&p_red), 0);
  if (ngreen == 0)
    ngreen = ply_set_read_cb(ply, "vertex", "g", read_color, (void*)(&p_green), 0);
  if (nblue == 0)
    nblue = ply_set_read_cb(ply, "vertex", "b", read_color, (void*)(&p_blue), 0);
  if (nalpha == 0)
    nalpha = ply_set_read_cb(ply, "vertex", "a", read_color, (void*)(&p_alpha), 0);

  if (nred == 0)
    nred = ply_set_read_cb(ply, "vertex", "diffuse_red", read_color, (void*)(&p_red), 0);
  if (ngreen == 0)
    ngreen = ply_set_read_cb(ply, "vertex", "diffuse_green", read_color, (void*)(&p_green), 0);
  if (nblue == 0)
    nblue = ply_set_read_cb(ply, "vertex", "diffuse_blue", read_color, (void*)(&p_blue), 0);
  if (nalpha == 0)
    nalpha = ply_set_read_cb(ply, "vertex", "diffuse_alpha", read_color, (void*)(&p_alpha), 0);

  if (nred > 0 || ngreen > 0 || nblue > 0 || nalpha > 0)
    {
    if ((nred && ((uint32_t)nred != *nr_of_vertices)) || (ngreen && ((uint32_t)ngreen != *nr_of_vertices)) || (nblue && ((uint32_t)nblue != *nr_of_vertices)) || (nalpha && ((uint32_t)nalpha != *nr_of_vertices)))
      {
      if (*nr_of_vertices)
        {
        free(*vertices);
        free(*vertex_normals);
        *vertices = NULL;
        *vertex_normals = NULL;
        *nr_of_vertices = 0;
        }
      ply_close(ply);
      return 0;
      }
    *vertex_colors = (uint32_t*)trico_malloc(*nr_of_vertices * sizeof(uint32_t));
    uint32_t* p_clr = *vertex_colors;
    for (uint32_t c = 0; c < *nr_of_vertices; ++c)
      *p_clr++ = 0xffffffff;
    }

  p_red = (uint8_t*)(*vertex_colors);
  p_green = p_red + 1;
  p_blue = p_red + 2;
  p_alpha = p_red + 3;

  uint32_t* p_tria_index = NULL;

  long ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", read_face, (void*)(&p_tria_index), 0);
  if (ntriangles == 0)
    ntriangles = ply_set_read_cb(ply, "face", "vertex_index", read_face, (void*)(&p_tria_index), 0);

  *nr_of_triangles = (uint32_t)ntriangles;

  if (ntriangles > 0)
    *triangles = (uint32_t*)trico_malloc(*nr_of_triangles * 3 * sizeof(uint32_t));

  p_tria_index = (uint32_t*)(*triangles);

  float* p_uv = NULL;

  long ntexcoords = ply_set_read_cb(ply, "face", "texcoord", read_texcoord, (void*)(&p_uv), 0);

  if (ntexcoords && ((uint32_t)ntexcoords != *nr_of_triangles))
    {
    if (*nr_of_vertices)
      {
      free(*vertices);
      free(*vertex_normals);
      free(*vertex_colors);
      *vertices = NULL;
      *vertex_normals = NULL;
      *vertex_colors = NULL;
      *nr_of_vertices = 0;
      }
    if (*nr_of_triangles)
      {
      free(*triangles);
      *triangles = NULL;
      *nr_of_triangles = 0;
      }
    ply_close(ply);
    return 0;
    }

  if (ntexcoords > 0)
    *texcoords = (float*)trico_malloc(*nr_of_triangles * 6 * sizeof(float));

  p_uv = (float*)(*texcoords);

  if (!ply_read(ply))
    return 0;

  ply_close(ply);

  return 1;
  }

int trico_write_ply(const uint32_t nr_of_vertices, const float* vertices, const float* vertex_normals, const uint32_t* vertex_colors, const uint32_t nr_of_triangles, const uint32_t* triangles, const float* texcoords, const char* filename)
  {
  if (!vertices)
    return 0;
  if (nr_of_vertices == 0)
    return 0;

  FILE* fp = fopen(filename, "wb");

  if (!fp)
    return 0;

  fprintf(fp, "ply\n");
  int n = 1;
  if (*(char *)&n == 1)
    fprintf(fp, "format binary_little_endian 1.0\n");
  else
    fprintf(fp, "format binary_big_endian 1.0\n");

  fprintf(fp, "element vertex %d\n", nr_of_vertices);
  fprintf(fp, "property float x\n");
  fprintf(fp, "property float y\n");
  fprintf(fp, "property float z\n");

  if (vertex_normals)
    {
    fprintf(fp, "property float nx\n");
    fprintf(fp, "property float ny\n");
    fprintf(fp, "property float nz\n");
    }

  if (vertex_colors)
    {
    fprintf(fp, "property uchar red\n");
    fprintf(fp, "property uchar green\n");
    fprintf(fp, "property uchar blue\n");
    fprintf(fp, "property uchar alpha\n");
    }

  if (nr_of_triangles && triangles)
    {
    fprintf(fp, "element face %d\n", nr_of_triangles);
    fprintf(fp, "property list uchar int vertex_indices\n");
    if (texcoords)
      fprintf(fp, "property list uchar float texcoord\n");
    }
  fprintf(fp, "end_header\n");

  for (uint32_t i = 0; i < nr_of_vertices; ++i)
    {
    fwrite(vertices + 3 * i, sizeof(float), 3, fp);
    if (vertex_normals)
      fwrite(vertex_normals + 3 * i, sizeof(float), 3, fp);
    if (vertex_colors)
      fwrite(vertex_colors + i, sizeof(uint32_t), 1, fp);
    }
  const unsigned char tria_size = 3;
  const unsigned char texcoord_size = 6;
  for (uint32_t i = 0; i < nr_of_triangles; ++i)
    {
    fwrite(&tria_size, 1, 1, fp);
    fwrite(triangles + 3 * i, sizeof(uint32_t), 3, fp);
    if (texcoords)
      {
      fwrite(&texcoord_size, 1, 1, fp);
      fwrite(texcoords + 6 * i, sizeof(float), 6, fp);
      }
    }
  fclose(fp);
  return 1;
  }