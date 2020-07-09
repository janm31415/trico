#include "floating_point_stream_compression_2.h"

#include "alloc.h"

namespace trico
  {

#define HASH_SIZE_EXPONENT 20
#define HASH_SIZE (1 << HASH_SIZE_EXPONENT)
#define HASH_MASK (HASH_SIZE - 1)



    /*
    High Throughput Compression of Double-Precision Floating-Point Data.
    Martin Burtscher and Paruj Ratanaworabhan.

    Adapted to 32-bit floating point values.
    */

  namespace
    {
    inline void fill_code(uint8_t** out, uint32_t* xor1, uint32_t* xor2, uint32_t* bcode)
      {
      uint32_t bc = (bcode[7] << 21) | (bcode[6] << 18) | (bcode[5] << 15) | (bcode[4] << 12) | (bcode[3] << 9) | (bcode[2] << 6) | (bcode[1] << 3) | bcode[0];

      *(*out)++ = (bc >> 16);
      *(*out)++ = (bc >> 8) & 0xff;
      *(*out)++ = bc & 0xff;

      for (uint32_t k = 0; k < 8; ++k)
        {
        switch (bcode[k])
          {
          case 0:
          {
          break;
          }
          case 1:
          {
          *(*out)++ = (uint8_t)(xor1[k]);
          break;
          }
          case 2:
          {
          *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
          break;
          }
          case 3:
          {
          *(*out)++ = (uint8_t)((xor1[k] >> 16));
          *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
          break;
          }
          case 4:
          {
          *(*out)++ = (uint8_t)((xor1[k] >> 24));
          *(*out)++ = (uint8_t)((xor1[k] >> 16) & 0xff);
          *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
          break;
          }
          case 5:
          {
          *(*out)++ = (uint8_t)(xor2[k]);
          break;
          }
          case 6:
          {
          *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
          break;
          }
          case 7:
          {
          *(*out)++ = (uint8_t)((xor2[k] >> 16));
          *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
          *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
          break;
          }
          }
        }
      }

    inline uint32_t compute_hash(uint32_t hash, uint32_t value)
      {
      return ((value >> (32 - HASH_SIZE_EXPONENT))) & HASH_MASK;
      }

    }

  void compress_2(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats)
    {
    uint32_t max_size = (number_of_floats) * sizeof(float) + 3*(number_of_floats + 7) / 8 ; // theoretical maximum
    *out = (uint8_t*)trico_malloc(max_size);

    uint32_t* hash_table_1 = (uint32_t*)calloc(HASH_SIZE, 4);
    uint32_t* hash_table_2 = (uint32_t*)calloc(HASH_SIZE, 4);

    uint32_t value;
    uint32_t stride;
    uint32_t last_value = 0;
    uint32_t hash1 = 0;
    uint32_t hash2 = 0;
    uint32_t prediction1 = 0;
    uint32_t prediction2 = 0;
    uint32_t xor1[8];
    uint32_t xor2[8];
    uint32_t bcode[8];
    uint32_t j;
    uint8_t* p_out = *out;
    
    *p_out++ = (uint8_t)((number_of_floats >> 24));
    *p_out++ = (uint8_t)((number_of_floats >> 16) & 0xff);
    *p_out++ = (uint8_t)((number_of_floats >> 8) & 0xff);
    *p_out++ = (uint8_t)((number_of_floats) & 0xff);

    for (uint32_t i = 0; i < number_of_floats; ++i)
      {
      j = i & 7;
      value = *reinterpret_cast<const uint32_t*>(input++);

      xor1[j] = value ^ prediction1;
      hash_table_1[hash1] = value;
      hash1 = compute_hash(hash1, value);
      prediction1 = hash_table_1[hash1];

      stride = value - last_value;
      xor2[j] = value ^ (last_value + prediction2);
      last_value = value;
      hash_table_2[hash2] = stride;
      hash2 = compute_hash(hash2, stride);
      prediction2 = hash_table_2[hash2];


      bcode[j] = 4; // 4 bytes
      if (0 == xor1[j])
        {
        bcode[j] = 0; // 0 bytes
        }
      else if (0 == (xor1[j] >> 8))
        {
        bcode[j] = 1; // 1 byte
        }
      else if (0 == (xor1[j] >> 16))
        {
        bcode[j] = 2; // 2 bytes
        if (0 == (xor2[j] >> 8))
          {
          bcode[j] = 5; // 1 byte
          }
        }
      else if (0 == (xor1[j] >> 24))
        {
        bcode[j] = 3; // 3 bytes
        if (0 == (xor2[j] >> 8))
          {
          bcode[j] = 5; // 1 byte
          }
        else if (0 == (xor2[j] >> 16))
          {
          bcode[j] = 6; // 2 bytes
          }
        }  
      else // 4 bytes
        {
        if (0 == (xor2[j] >> 8))
          {
          bcode[j] = 5; // 1 byte
          }
        else if (0 == (xor2[j] >> 16))
          {
          bcode[j] = 6; // 2 bytes
          }
        else if (0 == (xor2[j] >> 24))
          {
          bcode[j] = 7; // 3 bytes
          }
        }

      if (j == 7)
        {
        fill_code(&p_out, xor1, xor2, bcode);
        }
      }
    for (uint32_t l = j + 1; l < 8; ++l)
      {
      bcode[l] = 1;
      xor1[l] = 0;
      }
    if (j != 7)
      {
      fill_code(&p_out, xor1, xor2, bcode);
      }

    *nr_of_compressed_bytes = p_out - *out;
    free(hash_table_1);
    free(hash_table_2);
    *out = (uint8_t*)trico_realloc(*out, *nr_of_compressed_bytes);
    }

  void decompress_2(uint32_t* number_of_floats, float** out, const uint8_t* compressed)
    {
    uint32_t* hash_table_1 = (uint32_t*)calloc(HASH_SIZE, 4);
    uint32_t* hash_table_2 = (uint32_t*)calloc(HASH_SIZE, 4);

    *number_of_floats = ((uint32_t)(*compressed++)) << 24;
    *number_of_floats |= ((uint32_t)(*compressed++)) << 16;
    *number_of_floats |= ((uint32_t)(*compressed++)) << 8;
    *number_of_floats |= ((uint32_t)(*compressed++));
    *out = (float*)trico_malloc(*number_of_floats * sizeof(float));

    uint32_t bc;
    uint32_t bcode[8];
    uint32_t xor[8];      
    uint32_t value;
    uint32_t hash1 = 0;
    uint32_t prediction1 = 0;
    uint32_t hash2 = 0;
    uint32_t prediction2 = 0;
    uint32_t stride;
    uint32_t last_value = 0;
    uint32_t* p_out = reinterpret_cast<uint32_t*>(*out);

    const uint32_t cnt = *number_of_floats / 8;
    for (uint32_t q = 0; q < cnt; ++q)
      {
      bc = ((uint32_t)(*compressed++)) << 16;
      bc |= ((uint32_t)(*compressed++)) << 8;
      bc |= (*compressed++);
      for (int j = 0; j < 8; ++j)
        {
        uint8_t b = (bc >> (j*3)) & 7;
        bcode[j] = b;
        switch (b)
          {
          case 0:
          {
          xor[j] = 0;
          break;
          }
          case 1:
          {          
          xor[j] = ((uint32_t)(*compressed++));
          break;
          }
          case 2:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 3:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 4:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 24;
          xor[j] |= ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 5:
          {
          xor[j] = ((uint32_t)(*compressed++));
          break;
          }
          case 6:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 7:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          }
        }
      for (int j = 0; j < 8; ++j)
        {
        if (bcode[j] > 4)
          prediction1 = prediction2;

        value = xor[j] ^ prediction1;

        hash_table_1[hash1] = value;
        hash1 = compute_hash(hash1, value);
        prediction1 = hash_table_1[hash1];

        stride = value - last_value;
        hash_table_2[hash2] = stride;
        hash2 = compute_hash(hash2, stride);
        prediction2 = value + hash_table_2[hash2];
        last_value = value;

        *p_out++ = value;
        }
      }
    
    if (*number_of_floats & 7)
      {
      bc = ((uint32_t)(*compressed++)) << 16;
      bc |= ((uint32_t)(*compressed++)) << 8;
      bc |= (*compressed++);
      int max_j = 8;
      for (int j = 0; j < max_j; ++j)
        {
        uint8_t b = (bc >> (j * 3)) & 7;
        bcode[j] = b;
        switch (b)
          {
          case 0:
          {
          xor[j] = 0;
          break;
          }
          case 1:
          {
          xor[j] = ((uint32_t)(*compressed++));
          if (xor[j] == 0)
            max_j = j;
          break;
          }
          case 2:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 3:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 4:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 24;
          xor[j] |= ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 5:
          {
          xor[j] = ((uint32_t)(*compressed++));
          break;
          }
          case 6:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          case 7:
          {
          xor[j] = ((uint32_t)(*compressed++)) << 16;
          xor[j] |= ((uint32_t)(*compressed++)) << 8;
          xor[j] |= ((uint32_t)(*compressed++));
          break;
          }
          }
        }
      for (int j = 0; j < max_j; ++j)
        {
        if (bcode[j] > 4)
          prediction1 = prediction2;

        value = xor[j] ^ prediction1;

        hash_table_1[hash1] = value;
        hash1 = compute_hash(hash1, value);
        prediction1 = hash_table_1[hash1];

        stride = value - last_value;
        hash_table_2[hash2] = stride;
        hash2 = compute_hash(hash2, stride);
        prediction2 = value + hash_table_2[hash2];
        last_value = value;

        *p_out++ = value;
        }
      }

    free(hash_table_1);
    free(hash_table_2);
    }
  }