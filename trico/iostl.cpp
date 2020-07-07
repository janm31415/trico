#include "iostl.h"
#include "alloc.h"
#include <cassert>
#include <array>
#include <vector>

namespace trico
  {

  namespace
    {

    void remove_duplicate_vertices(uint32_t* nr_of_vertices, float** vertices, uint32_t nr_of_triangles, uint32_t** triangles)
      {
      if (nr_of_triangles == 0)
        return;
      std::vector<std::array<float, 4>> vert;
      vert.reserve(nr_of_triangles * 3);
      for (uint32_t i = 0; i < nr_of_triangles; ++i)
        {
        const uint32_t id0 = i * 3;
        const uint32_t id1 = id0 + 1;
        const uint32_t id2 = id1 + 1;
        const uint32_t v0 = (*triangles)[id0];
        const uint32_t v1 = (*triangles)[id1];
        const uint32_t v2 = (*triangles)[id2];

        vert.push_back({ (*vertices)[v0 * 3], (*vertices)[v0 * 3 + 1], (*vertices)[v0 * 3 + 2], *reinterpret_cast<const float*>(&id0) });
        vert.push_back({ (*vertices)[v1 * 3], (*vertices)[v1 * 3 + 1], (*vertices)[v1 * 3 + 2], *reinterpret_cast<const float*>(&id1) });
        vert.push_back({ (*vertices)[v2 * 3], (*vertices)[v2 * 3 + 1], (*vertices)[v2 * 3 + 2], *reinterpret_cast<const float*>(&id2) });
        }

      auto less_fie = [](const std::array<float, 4>& left, const std::array<float, 4>& right)
        {
        return left[0] == right[0] ? (left[1] == right[1] ? left[2] < right[2] : left[1] < right[1]) : left[0] < right[0];
        };

      auto equal_fie = [](const std::array<float, 4>& left, const std::array<float, 4>& right)
        {
        return (left[0] == right[0] && left[1] == right[1] && left[2] == right[2]);
        };

      std::sort(vert.begin(), vert.end(), less_fie);      

      auto first = vert.begin();
      auto last = vert.end();

      *nr_of_vertices = 0;

      auto result = first;
      (*vertices)[*nr_of_vertices * 3] = (*result)[0];
      (*vertices)[*nr_of_vertices * 3 + 1] = (*result)[1];
      (*vertices)[*nr_of_vertices * 3 + 2] = (*result)[2];
      (*triangles)[*reinterpret_cast<uint32_t*>(&((*result)[3]))] = *nr_of_vertices;
      while (++first != last)
        {
        if (!equal_fie(*result, *first))
          {
          while (++result != first)
            {
            (*triangles)[*reinterpret_cast<uint32_t*>(&((*result)[3]))] = *nr_of_vertices;
            }
          ++(*nr_of_vertices);
          (*vertices)[*nr_of_vertices * 3] = (*result)[0];
          (*vertices)[*nr_of_vertices * 3 + 1] = (*result)[1];
          (*vertices)[*nr_of_vertices * 3 + 2] = (*result)[2];
          (*triangles)[*reinterpret_cast<uint32_t*>(&((*result)[3]))] = *nr_of_vertices;
          }
        }
      while (++result != first)
        {
        (*triangles)[*reinterpret_cast<uint32_t*>(&((*result)[3]))] = *nr_of_vertices;
        }
      ++(*nr_of_vertices);
      }
    }

  int read_stl(uint32_t* nr_of_vertices, float** vertices, uint32_t* nr_of_triangles, uint32_t** triangles, const char* filename)
    {
    *vertices = nullptr;
    *triangles = nullptr;
    *nr_of_vertices = 0;
    *nr_of_triangles = 0;
    FILE* inputfile = nullptr;
    auto err = fopen_s(&inputfile, filename, "rb");
    if (err != 0)
      return false;

    if (!inputfile)
      return false;

    char buffer[80];
    fread(buffer, 1, 80, inputfile);

    if (buffer[0] == 's' && buffer[1] == 'o' && buffer[2] == 'l' && buffer[3] == 'i' && buffer[4] == 'd')
      {
      fclose(inputfile);
      return false;
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
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[12]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[16]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[20]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[24]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[28]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[32]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[36]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[40]));
      *vert_it++ = *reinterpret_cast<float*>(&(buffer[44]));
      *tria_it++ = (*nr_of_vertices)++;
      *tria_it++ = (*nr_of_vertices)++;
      *tria_it++ = (*nr_of_vertices)++;
      }
    fclose(inputfile);
    if (count_triangles != *nr_of_triangles)
      return 1;

    remove_duplicate_vertices(nr_of_vertices, vertices, *nr_of_triangles, triangles);
    *vertices = (float*)trico_realloc(*vertices, *nr_of_vertices * 3 * sizeof(float));
    return 0;
    } 

  }