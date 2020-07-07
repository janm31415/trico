#include "floating_point_stream_compression.h"

#include "alloc.h"

namespace trico
  {

#define HASH_SIZE 1 << 12

  /*
  High Throughput Compression of Double-Precision Floating-Point Data.  Martin Burtscher and Paruj Ratanaworabhan.

  Adapted to 32-bit floating point values.
  */

  namespace
    {
    inline void fill_code(uint8_t** out, uint32_t* xor, uint32_t* bcode)
      {
      *(*out) = (bcode[3] << 6) | (bcode[2] << 4) | (bcode[1] << 2) | bcode[0];
      ++(*out);
      for (uint32_t k = 0; k < 4; ++k)
        {
        switch (bcode[k])
          {
          case 0:
          {
          break;
          }
          case 1:
          {
          *(*out)++ = (uint8_t)(xor[k]);
          break;
          }
          case 2:
          {
          *(*out)++ = (uint8_t)((xor[k] >> 16));
          *(*out)++ = (uint8_t)((xor[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor[k]) & 0xff);
          break;
          }
          case 3:
          {
          *(*out)++ = (uint8_t)((xor[k] >> 24));
          *(*out)++ = (uint8_t)((xor[k] >> 16) & 0xff);
          *(*out)++ = (uint8_t)((xor[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor[k]) & 0xff);
          break;
          }
          }
        }
      }

    inline uint32_t compute_hash(uint32_t hash, uint32_t value, uint32_t hash_size)
      {
      //return ((hash << 3) ^ (value >> 23)) & (hash_size - 1);
      //return ((hash << 2) ^ (value)) & (hash_size - 1);
      return (value) & (hash_size - 1);
      }
    }

  void compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats)
    {
    uint32_t max_size = (number_of_floats + 3) * sizeof(float) + (number_of_floats + 3) / 4; // theoretical maximum
    *out = (uint8_t*)trico_malloc(max_size);

    uint32_t hash_size = HASH_SIZE;
    uint32_t* hash_table = (uint32_t*)calloc(hash_size, 4);

    uint32_t value;
    uint32_t hash = 0;
    uint32_t prediction = 0;
    uint32_t xor[4];
    uint32_t bcode[4];
    uint32_t j;
    uint8_t* p_out = *out;

    *p_out++ = (uint8_t)((number_of_floats >> 24));
    *p_out++ = (uint8_t)((number_of_floats >> 16) & 0xff);
    *p_out++ = (uint8_t)((number_of_floats >> 8) & 0xff);
    *p_out++ = (uint8_t)((number_of_floats) & 0xff);

    for (uint32_t i = 0; i < number_of_floats; ++i)
      {
      j = i & 3;
      value = *reinterpret_cast<const uint32_t*>(input++);
      xor[j] = value ^ prediction;
      hash_table[hash] = value;
      //hash = ((hash << 3) ^ (value >> 23)) & (hash_size - 1);
      hash = compute_hash(hash, value, hash_size);
      prediction = hash_table[hash];
      bcode[j] = 3; // 4 bytes
      if (0 == (xor[j] >> 24))
        bcode[j] = 2; // 3 bytes
      if (0 == (xor[j] >> 8))
        bcode[j] = 1; // 1 byte
      if (0 == xor[j])
        bcode[j] = 0; // 0 bytes
      if (j == 3)
        {
        fill_code(&p_out, xor, bcode);        
        }
      }
    for (uint32_t l = j + 1; l < 4; ++l)
      {
      bcode[l] = 1; 
      xor[l] = 0;
      }
    if (j != 3)
      {
      fill_code(&p_out, xor, bcode);      
      }

    *nr_of_compressed_bytes = p_out - *out;
    free(hash_table);
    *out = (uint8_t*)trico_realloc(*out, *nr_of_compressed_bytes);
    }

  void decompress(uint32_t* number_of_floats, float** out, const uint8_t* compressed)
    {
    uint32_t hash_size = HASH_SIZE;
    uint32_t* hash_table = (uint32_t*)calloc(hash_size, 4);

    *number_of_floats = ((uint32_t)(*compressed++)) << 24;
    *number_of_floats |= ((uint32_t)(*compressed++)) << 16;
    *number_of_floats |= ((uint32_t)(*compressed++)) << 8;
    *number_of_floats |= ((uint32_t)(*compressed++));
    *out = (float*)trico_malloc(*number_of_floats * sizeof(float));

    uint8_t bcode; 
    uint32_t xor[4];
    uint32_t value;
    uint32_t hash = 0;
    uint32_t prediction = 0;

    uint32_t* p_out = reinterpret_cast<uint32_t*>(*out);

    const uint32_t cnt = *number_of_floats / 4;
    for (uint32_t q = 0; q < cnt; ++q)
      {
      bcode = *compressed++;
      for (int j = 0; j < 4; ++j)
        {
        uint8_t b = (bcode >> (j << 1)) & 3;
        switch (b)
          {
          case 0:
          {
          xor[j] = 0;
          break;
          }
          case 1:
          {
          xor[j] = *compressed++;
          break;
          }
          case 2:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 3:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 24;
          xor[j] |= ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          }
        }
      for (int j = 0; j < 4; ++j)
        {
        value = xor[j] ^ prediction;
        hash_table[hash] = value;
        //hash = ((hash << 3) ^ (value >> 23)) & (hash_size - 1);
        hash = compute_hash(hash, value, hash_size);
        prediction = hash_table[hash];
        *p_out++ = value;
        }
      }
    if (*number_of_floats & 3)
      {
      bcode = *compressed++;
      int max_j = 4;
      for (int j = 0; j < max_j; ++j)
        {
        uint8_t b = (bcode >> (j << 1)) & 3;
        switch (b)
          {
          case 0:
          {
          xor[j] = 0;
          break;
          }
          case 1:
          {
          xor[j] = *compressed++;
          if (xor[j] == 0)
            max_j = j;
          break;
          }
          case 2:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 3:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 24;
          xor[j] |= ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          }
        }
      for (int j = 0; j < max_j; ++j)
        {
        value = xor[j] ^ prediction;
        hash_table[hash] = value;
        //hash = ((hash << 3) ^ (value >> 23)) & (hash_size - 1);
        hash = compute_hash(hash, value, hash_size);
        prediction = hash_table[hash];
        *p_out++ = value;
        }
      }

    free(hash_table);
    }

  }