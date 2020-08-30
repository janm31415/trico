#include "iostl.h"
#include "alloc.h"
#include <stdio.h>

static inline int trico_less_vertices(float* left, float* right)
  {
  if (left[0] == right[0])
    {
    if (left[1] == right[1])
      return left[2] < right[2] ? 1 : 0;
    else
      return left[1] < right[1] ? 1 : 0;
    }
  else
    return left[0] < right[0] ? 1 : 0;
  }

static inline int trico_equal_vertices(float* left, float* right)
  {
  if ((left[0] == right[0]) && (left[1] == right[1]) && (left[2] == right[2]))
    return 1;
  return 0;
  }

static inline void trico_swap_vertices(float* a, float* b)
  {
  for (int j = 0; j < 4; ++j)
    {
    float temp = a[j];
    a[j] = b[j];
    b[j] = temp;
    }
  }

static uint64_t trico_partition_vertices(float* L, int64_t left, int64_t right)
  {
  int64_t pivot = right;
  float p_val[4] = { L[pivot * 4], L[pivot * 4 + 1], L[pivot * 4 + 2], L[pivot * 4 + 3] };

  int64_t i = (left - 1);
  for (int64_t j = left; j < right; ++j)
    {
    // If current element is smaller than the pivot
    if (trico_less_vertices(L + j * 4, p_val))
      {
      ++i;    // increment index of smaller element
      trico_swap_vertices(L + i * 4, L + j * 4);
      }
    }
  trico_swap_vertices(L + (i + 1) * 4, L + right * 4);
  return (i + 1);
  }

static void trico_quicksort_vertices(float* L, int64_t start, int64_t end)
  {
  if (start >= end)
    return;
  int64_t splitPoint = trico_partition_vertices(L, start, end);
  trico_quicksort_vertices(L, start, splitPoint - 1);
  trico_quicksort_vertices(L, splitPoint + 1, end);
  }

void trico_remove_duplicate_vertices(uint32_t* nr_of_vertices, float** vertices, uint32_t nr_of_triangles, uint32_t** triangles)
  {
  if (nr_of_triangles == 0)
    return;

  float* vert = (float*)trico_malloc(nr_of_triangles * 3 * sizeof(float) * 4);

  float* p_vert = vert;
  for (uint32_t i = 0; i < nr_of_triangles; ++i)
    {
    const uint32_t id0 = i * 3;
    const uint32_t id1 = id0 + 1;
    const uint32_t id2 = id1 + 1;
    const uint32_t v0 = (*triangles)[id0];
    const uint32_t v1 = (*triangles)[id1];
    const uint32_t v2 = (*triangles)[id2];


    *p_vert++ = (*vertices)[v0 * 3];
    *p_vert++ = (*vertices)[v0 * 3 + 1];
    *p_vert++ = (*vertices)[v0 * 3 + 2];
    *p_vert++ = *(const float*)(&id0);

    *p_vert++ = (*vertices)[v1 * 3];
    *p_vert++ = (*vertices)[v1 * 3 + 1];
    *p_vert++ = (*vertices)[v1 * 3 + 2];
    *p_vert++ = *(const float*)(&id1);

    *p_vert++ = (*vertices)[v2 * 3];
    *p_vert++ = (*vertices)[v2 * 3 + 1];
    *p_vert++ = (*vertices)[v2 * 3 + 2];
    *p_vert++ = *(const float*)(&id2);
    }

  trico_quicksort_vertices(vert, 0, (3 * nr_of_triangles) - 1);

  float* first = vert;
  float* last = vert + 3 * nr_of_triangles * 4;

  *nr_of_vertices = 0;

  float* result = first;
  (*vertices)[*nr_of_vertices * 3] = result[0];
  (*vertices)[*nr_of_vertices * 3 + 1] = result[1];
  (*vertices)[*nr_of_vertices * 3 + 2] = result[2];
  (*triangles)[*(uint32_t*)(&(result[3]))] = *nr_of_vertices;
  while ((first += 4) != last)
    {
    if (trico_equal_vertices(result, first) == 0)
      {
      while ((result += 4) != first)
        {
        (*triangles)[*(uint32_t*)(&(result[3]))] = *nr_of_vertices;
        }
      ++(*nr_of_vertices);
      (*vertices)[*nr_of_vertices * 3] = result[0];
      (*vertices)[*nr_of_vertices * 3 + 1] = result[1];
      (*vertices)[*nr_of_vertices * 3 + 2] = result[2];
      (*triangles)[*(uint32_t*)(&(result[3]))] = *nr_of_vertices;
      }
    }
  while ((result += 4) != first)
    {
    (*triangles)[*(uint32_t*)(&(result[3]))] = *nr_of_vertices;
    }
  ++(*nr_of_vertices);

  trico_free(vert);
  }


int read_stl(uint32_t* nr_of_vertices, float** vertices, uint32_t* nr_of_triangles, uint32_t** triangles, const char* filename)
  {
  *vertices = NULL;
  *triangles = NULL;
  *nr_of_vertices = 0;
  *nr_of_triangles = 0;
  FILE* inputfile = fopen(filename, "rb");
  if (!inputfile)
    return 0;

  if (!inputfile)
    return 0;

  char buffer[80];
  fread(buffer, 1, 80, inputfile);

  if (buffer[0] == 's' && buffer[1] == 'o' && buffer[2] == 'l' && buffer[3] == 'i' && buffer[4] == 'd')
    {
    fclose(inputfile);
    return 0;
    }

  fread((void*)(nr_of_triangles), sizeof(uint32_t), 1, inputfile);
  *triangles = (uint32_t*)trico_malloc(*nr_of_triangles * 3 * sizeof(uint32_t));
  uint32_t count_triangles = 0;
  *nr_of_vertices = 0;
  *vertices = (float*)trico_malloc(*nr_of_triangles * 9 * sizeof(float));

  uint32_t* tria_it = *triangles;
  float* vert_it = *vertices;
  while (!feof(inputfile) && count_triangles < *nr_of_triangles)
    {
    ++count_triangles;
    fread(buffer, 1, 50, inputfile);
    *vert_it++ = *(float*)(&(buffer[12]));
    *vert_it++ = *(float*)(&(buffer[16]));
    *vert_it++ = *(float*)(&(buffer[20]));
    *vert_it++ = *(float*)(&(buffer[24]));
    *vert_it++ = *(float*)(&(buffer[28]));
    *vert_it++ = *(float*)(&(buffer[32]));
    *vert_it++ = *(float*)(&(buffer[36]));
    *vert_it++ = *(float*)(&(buffer[40]));
    *vert_it++ = *(float*)(&(buffer[44]));
    *tria_it++ = (*nr_of_vertices)++;
    *tria_it++ = (*nr_of_vertices)++;
    *tria_it++ = (*nr_of_vertices)++;
    }
  fclose(inputfile);
  if (count_triangles != *nr_of_triangles)
    return 0;

  trico_remove_duplicate_vertices(nr_of_vertices, vertices, *nr_of_triangles, triangles);
  *vertices = (float*)trico_realloc(*vertices, *nr_of_vertices * 3 * sizeof(float));
  return 1;
  }

