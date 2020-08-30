#include "floating_point_stream_compression.h"

#include "alloc.h"

/*
High Throughput Compression of Double-Precision Floating-Point Data.
Martin Burtscher and Paruj Ratanaworabhan.

Adapted to 32-bit floating point values.
*/

inline void trico_fill_code(uint8_t** out, uint32_t* xor1, uint32_t* xor2, uint32_t* bcode)
  {
  uint32_t bc = (bcode[7] << 21) | (bcode[6] << 18) | (bcode[5] << 15) | (bcode[4] << 12) | (bcode[3] << 9) | (bcode[2] << 6) | (bcode[1] << 3) | bcode[0];

  *(*out)++ = (uint8_t)(bc >> 16);
  *(*out)++ = (uint8_t)((bc >> 8) & 0xff);
  *(*out)++ = (uint8_t)(bc & 0xff);

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

inline uint32_t trico_compute_hash1_32(uint32_t hash, uint32_t value, uint32_t hash_size_exponent, uint32_t hash_mask)
  {
  return ((hash << (hash_size_exponent)) ^ (value >> (32 - hash_size_exponent))) & hash_mask;
  }

inline uint32_t trico_compute_hash2_32(uint32_t hash, uint32_t value, uint32_t hash_size_exponent, uint32_t hash_mask)
  {
  return ((hash << (hash_size_exponent / 2)) ^ (value >> (32 - hash_size_exponent))) & hash_mask;
  }



void trico_compress(uint32_t* nr_of_compressed_bytes, uint8_t** out, const float* input, const uint32_t number_of_floats, uint32_t hash1_size_exponent, uint32_t hash2_size_exponent)
  {
  hash1_size_exponent = (hash1_size_exponent >> 1) << 1;
  hash2_size_exponent = (hash2_size_exponent >> 1) << 1;
  if (hash1_size_exponent > 30)
    hash1_size_exponent = 30;
  if (hash2_size_exponent > 30)
    hash2_size_exponent = 30;

  uint32_t max_size = (number_of_floats) * sizeof(float) + 3 * (number_of_floats + 7) / 8 + (number_of_floats & 7); // theoretical maximum
  *out = (uint8_t*)trico_malloc(max_size);


  const uint32_t hash1_size = 1 << hash1_size_exponent;
  const uint32_t hash2_size = 1 << hash2_size_exponent;
  const uint32_t hash1_mask = hash1_size - 1;
  const uint32_t hash2_mask = hash2_size - 1;

  uint32_t* hash_table_1 = (uint32_t*)calloc(hash1_size, 4);
  uint32_t* hash_table_2 = (uint32_t*)calloc(hash2_size, 4);

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
  uint32_t j = 0;
  uint8_t* p_out = *out;

  uint8_t hash_info = (uint8_t)(((hash1_size_exponent >> 1) << 4) | (hash2_size_exponent >> 1));
  *p_out++ = hash_info;

  *p_out++ = (uint8_t)((number_of_floats >> 24));
  *p_out++ = (uint8_t)((number_of_floats >> 16) & 0xff);
  *p_out++ = (uint8_t)((number_of_floats >> 8) & 0xff);
  *p_out++ = (uint8_t)((number_of_floats) & 0xff);

  for (uint32_t i = 0; i < number_of_floats; ++i)
    {
    j = i & 7;
    value = *(const uint32_t*)(input++);

    xor1[j] = value ^ prediction1;
    hash_table_1[hash1] = value;
    hash1 = trico_compute_hash1_32(hash1, value, hash1_size_exponent, hash1_mask);
    prediction1 = hash_table_1[hash1];

    stride = value - last_value;
    xor2[j] = value ^ (last_value + prediction2);
    last_value = value;
    hash_table_2[hash2] = stride;
    hash2 = trico_compute_hash2_32(hash2, stride, hash2_size_exponent, hash2_mask);
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
      trico_fill_code(&p_out, xor1, xor2, bcode);
      }
    }
  for (uint32_t l = j + 1; l < 8; ++l)
    {
    bcode[l] = 1;
    xor1[l] = 0;
    }
  if (j != 7)
    {
    trico_fill_code(&p_out, xor1, xor2, bcode);
    }

  *nr_of_compressed_bytes = (uint32_t)(p_out - *out);
  free(hash_table_1);
  free(hash_table_2);
  *out = (uint8_t*)trico_realloc(*out, *nr_of_compressed_bytes);
  }

void trico_decompress(uint32_t* number_of_floats, float** out, const uint8_t* compressed)
  {
  uint8_t hash_info = *compressed++;

  uint32_t hash1_size_exponent = (hash_info >> 4) << 1;
  uint32_t hash2_size_exponent = (hash_info & 15) << 1;

  const uint32_t hash1_size = 1 << hash1_size_exponent;
  const uint32_t hash2_size = 1 << hash2_size_exponent;
  const uint32_t hash1_mask = hash1_size - 1;
  const uint32_t hash2_mask = hash2_size - 1;

  uint32_t* hash_table_1 = (uint32_t*)calloc(hash1_size, 4);
  uint32_t* hash_table_2 = (uint32_t*)calloc(hash2_size, 4);

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
  uint32_t* p_out = (uint32_t*)(*out);

  const uint32_t cnt = *number_of_floats / 8;
  for (uint32_t q = 0; q < cnt; ++q)
    {
    bc = ((uint32_t)(*compressed++)) << 16;
    bc |= ((uint32_t)(*compressed++)) << 8;
    bc |= (*compressed++);
    for (int j = 0; j < 8; ++j)
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
      hash1 = trico_compute_hash1_32(hash1, value, hash1_size_exponent, hash1_mask);
      prediction1 = hash_table_1[hash1];

      stride = value - last_value;
      hash_table_2[hash2] = stride;
      hash2 = trico_compute_hash2_32(hash2, stride, hash2_size_exponent, hash2_mask);
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
      hash1 = trico_compute_hash1_32(hash1, value, hash1_size_exponent, hash1_mask);
      prediction1 = hash_table_1[hash1];

      stride = value - last_value;
      hash_table_2[hash2] = stride;
      hash2 = trico_compute_hash2_32(hash2, stride, hash2_size_exponent, hash2_mask);
      prediction2 = value + hash_table_2[hash2];
      last_value = value;

      *p_out++ = value;
      }
    }

  free(hash_table_1);
  free(hash_table_2);
  }



inline void trico_fill_code_double(uint8_t** out, uint64_t* xor1, uint64_t* xor2, uint64_t* bcode)
  {
  uint8_t bc = (uint8_t)((bcode[1] << 4) | bcode[0]);

  *(*out)++ = bc;

  for (uint32_t k = 0; k < 2; ++k)
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
      *(*out)++ = (uint8_t)((xor1[k] >> 32));
      *(*out)++ = (uint8_t)((xor1[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
      break;
      }
      case 6:
      {
      *(*out)++ = (uint8_t)((xor1[k] >> 40));
      *(*out)++ = (uint8_t)((xor1[k] >> 32) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
      break;
      }
      case 7:
      {
      *(*out)++ = (uint8_t)((xor1[k] >> 48));
      *(*out)++ = (uint8_t)((xor1[k] >> 40) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 32) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
      break;
      }
      case 8:
      {
      *(*out)++ = (uint8_t)((xor1[k] >> 56));
      *(*out)++ = (uint8_t)((xor1[k] >> 48) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 40) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 32) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor1[k]) & 0xff);
      break;
      }
      case 9:
      {
      *(*out)++ = (uint8_t)(xor2[k]);
      break;
      }
      case 10:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      case 11:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 16));
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      case 12:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 24));
      *(*out)++ = (uint8_t)((xor2[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      case 13:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 32));
      *(*out)++ = (uint8_t)((xor2[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      case 14:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 40));
      *(*out)++ = (uint8_t)((xor2[k] >> 32) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      case 15:
      {
      *(*out)++ = (uint8_t)((xor2[k] >> 48));
      *(*out)++ = (uint8_t)((xor2[k] >> 40) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 32) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 24) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 16) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k] >> 8) & 0xff);
      *(*out)++ = (uint8_t)((xor2[k]) & 0xff);
      break;
      }
      }
    }
  }



inline uint64_t trico_compute_hash1_64(uint64_t hash, uint64_t value, uint64_t hash1_size_exponent, uint64_t hash1_mask)
  {
  return ((hash << hash1_size_exponent) ^ (value >> (64 - hash1_size_exponent))) & hash1_mask;
  }

inline uint64_t trico_compute_hash2_64(uint64_t hash, uint64_t value, uint64_t hash2_size_exponent, uint64_t hash2_mask)
  {
  return ((hash << hash2_size_exponent / 2) ^ (value >> (64 - hash2_size_exponent))) & hash2_mask;
  }


void trico_compress_double_precision(uint32_t* nr_of_compressed_bytes, uint8_t** out, const double* input, const uint32_t number_of_doubles, uint64_t hash1_size_exponent, uint64_t hash2_size_exponent)
  {
  hash1_size_exponent = (hash1_size_exponent >> 1) << 1;
  hash2_size_exponent = (hash2_size_exponent >> 1) << 1;
  if (hash1_size_exponent > 30)
    hash1_size_exponent = 30;
  if (hash2_size_exponent > 30)
    hash2_size_exponent = 30;

  uint32_t max_size = (number_of_doubles) * sizeof(double) + (number_of_doubles) / 2 + (number_of_doubles & 1); // theoretical maximum
  *out = (uint8_t*)trico_malloc(max_size);

  const uint64_t hash1_size = (uint64_t)1 << hash1_size_exponent;
  const uint64_t hash2_size = (uint64_t)1 << hash2_size_exponent;
  const uint64_t hash1_mask = hash1_size - 1;
  const uint64_t hash2_mask = hash2_size - 1;

  uint64_t* hash_table_1 = (uint64_t*)calloc(hash1_size, 8);
  uint64_t* hash_table_2 = (uint64_t*)calloc(hash2_size, 8);

  uint64_t value;
  uint64_t stride;
  uint64_t last_value = 0;
  uint64_t hash1 = 0;
  uint64_t hash2 = 0;
  uint64_t prediction1 = 0;
  uint64_t prediction2 = 0;
  uint64_t xor1[2];
  uint64_t xor2[2];
  uint64_t bcode[2];
  uint32_t j = 0;
  uint8_t* p_out = *out;

  uint8_t hash_info = (uint8_t)(((hash1_size_exponent >> 1) << 4) | (hash2_size_exponent >> 1));
  *p_out++ = hash_info;

  *p_out++ = (uint8_t)((number_of_doubles >> 24));
  *p_out++ = (uint8_t)((number_of_doubles >> 16) & 0xff);
  *p_out++ = (uint8_t)((number_of_doubles >> 8) & 0xff);
  *p_out++ = (uint8_t)((number_of_doubles) & 0xff);

  for (uint32_t i = 0; i < number_of_doubles; ++i)
    {
    j = i & 1;
    value = *(const uint64_t*)(input++);

    xor1[j] = value ^ prediction1;
    hash_table_1[hash1] = value;
    hash1 = trico_compute_hash1_64(hash1, value, hash1_size_exponent, hash1_mask);
    prediction1 = hash_table_1[hash1];

    stride = value - last_value;
    xor2[j] = value ^ (last_value + prediction2);
    last_value = value;
    hash_table_2[hash2] = stride;
    hash2 = trico_compute_hash2_64(hash2, stride, hash2_size_exponent, hash2_mask);
    prediction2 = hash_table_2[hash2];


    bcode[j] = 8; // 8 bytes
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
        bcode[j] = 9; // 1 byte
        }
      }
    else if (0 == (xor1[j] >> 24))
      {
      bcode[j] = 3; // 3 bytes
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      }
    else if (0 == (xor1[j] >> 32))
      {
      bcode[j] = 4; // 4 bytes
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      else if (0 == (xor2[j] >> 24))
        {
        bcode[j] = 11; // 3 bytes
        }
      }
    else if (0 == (xor1[j] >> 40))
      {
      bcode[j] = 5; // 5 bytes
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      else if (0 == (xor2[j] >> 24))
        {
        bcode[j] = 11; // 3 bytes
        }
      else if (0 == (xor2[j] >> 32))
        {
        bcode[j] = 12; // 4 bytes
        }
      }
    else if (0 == (xor1[j] >> 48))
      {
      bcode[j] = 6; // 6 bytes
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      else if (0 == (xor2[j] >> 24))
        {
        bcode[j] = 11; // 3 bytes
        }
      else if (0 == (xor2[j] >> 32))
        {
        bcode[j] = 12; // 4 bytes
        }
      else if (0 == (xor2[j] >> 40))
        {
        bcode[j] = 13; // 5 bytes
        }
      }
    else if (0 == (xor1[j] >> 56))
      {
      bcode[j] = 7; // 7 bytes
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      else if (0 == (xor2[j] >> 24))
        {
        bcode[j] = 11; // 3 bytes
        }
      else if (0 == (xor2[j] >> 32))
        {
        bcode[j] = 12; // 4 bytes
        }
      else if (0 == (xor2[j] >> 40))
        {
        bcode[j] = 13; // 5 bytes
        }
      else if (0 == (xor2[j] >> 48))
        {
        bcode[j] = 14; // 6 bytes
        }
      }
    else // 8 bytes
      {
      if (0 == (xor2[j] >> 8))
        {
        bcode[j] = 9; // 1 byte
        }
      else if (0 == (xor2[j] >> 16))
        {
        bcode[j] = 10; // 2 bytes
        }
      else if (0 == (xor2[j] >> 24))
        {
        bcode[j] = 11; // 3 bytes
        }
      else if (0 == (xor2[j] >> 32))
        {
        bcode[j] = 12; // 4 bytes
        }
      else if (0 == (xor2[j] >> 40))
        {
        bcode[j] = 13; // 5 bytes
        }
      else if (0 == (xor2[j] >> 48))
        {
        bcode[j] = 14; // 6 bytes
        }
      else if (0 == (xor2[j] >> 56))
        {
        bcode[j] = 15; // 7 bytes
        }
      }

    if (j == 1)
      {
      trico_fill_code_double(&p_out, xor1, xor2, bcode);
      }
    }
  if (j == 0)
    {
    bcode[1] = 1;
    xor1[1] = 0;
    trico_fill_code_double(&p_out, xor1, xor2, bcode);
    }

  *nr_of_compressed_bytes = (uint32_t)(p_out - *out);
  free(hash_table_1);
  free(hash_table_2);
  *out = (uint8_t*)trico_realloc(*out, *nr_of_compressed_bytes);
  }


void trico_decompress_double_precision(uint32_t* number_of_doubles, double** out, const uint8_t* compressed)
  {
  uint8_t hash_info = *compressed++;

  uint64_t hash1_size_exponent = (hash_info >> 4) << 1;
  uint64_t hash2_size_exponent = (hash_info & 15) << 1;

  const uint64_t hash1_size = (uint64_t)1 << hash1_size_exponent;
  const uint64_t hash2_size = (uint64_t)1 << hash2_size_exponent;
  const uint64_t hash1_mask = hash1_size - 1;
  const uint64_t hash2_mask = hash2_size - 1;

  uint64_t* hash_table_1 = (uint64_t*)calloc(hash1_size, 8);
  uint64_t* hash_table_2 = (uint64_t*)calloc(hash2_size, 8);

  *number_of_doubles = ((uint32_t)(*compressed++)) << 24;
  *number_of_doubles |= ((uint32_t)(*compressed++)) << 16;
  *number_of_doubles |= ((uint32_t)(*compressed++)) << 8;
  *number_of_doubles |= ((uint32_t)(*compressed++));
  *out = (double*)trico_malloc(*number_of_doubles * sizeof(double));

  uint64_t bc;
  uint64_t bcode[2];
  uint64_t xor[2];
  uint64_t value;
  uint64_t hash1 = 0;
  uint64_t prediction1 = 0;
  uint64_t hash2 = 0;
  uint64_t prediction2 = 0;
  uint64_t stride;
  uint64_t last_value = 0;
  uint64_t* p_out = (uint64_t*)(*out);

  const uint32_t cnt = *number_of_doubles / 2;
  for (uint32_t q = 0; q < cnt; ++q)
    {
    bc = ((uint32_t)(*compressed++));
    for (int j = 0; j < 2; ++j)
      {
      uint8_t b = (bc >> (j * 4)) & 15;
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
        xor[j] = ((uint64_t)(*compressed++));
        break;
        }
        case 2:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 3:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 4:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 5:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 6:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 7:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 8:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 56;
        xor[j] |= ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 9:
        {
        xor[j] = ((uint64_t)(*compressed++));
        break;
        }
        case 10:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 11:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 12:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 13:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 14:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 15:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        }
      }
    for (int j = 0; j < 2; ++j)
      {
      if (bcode[j] > 8)
        prediction1 = prediction2;

      value = xor[j] ^ prediction1;

      hash_table_1[hash1] = value;
      hash1 = trico_compute_hash1_64(hash1, value, hash1_size_exponent, hash1_mask);
      prediction1 = hash_table_1[hash1];

      stride = value - last_value;
      hash_table_2[hash2] = stride;
      hash2 = trico_compute_hash2_64(hash2, stride, hash2_size_exponent, hash2_mask);
      prediction2 = value + hash_table_2[hash2];
      last_value = value;

      *p_out++ = value;
      }
    }

  if (*number_of_doubles & 1)
    {
    bc = ((uint32_t)(*compressed++));
    int max_j = 2;
    for (int j = 0; j < max_j; ++j)
      {
      uint8_t b = (bc >> (j * 4)) & 15;
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
        xor[j] = ((uint64_t)(*compressed++));
        if (xor[j] == 0)
          max_j = j;
        break;
        }
        case 2:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 3:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 4:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 5:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 6:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 7:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 8:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 56;
        xor[j] |= ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 9:
        {
        xor[j] = ((uint64_t)(*compressed++));
        break;
        }
        case 10:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 11:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 12:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 13:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 14:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        case 15:
        {
        xor[j] = ((uint64_t)(*compressed++)) << 48;
        xor[j] |= ((uint64_t)(*compressed++)) << 40;
        xor[j] |= ((uint64_t)(*compressed++)) << 32;
        xor[j] |= ((uint64_t)(*compressed++)) << 24;
        xor[j] |= ((uint64_t)(*compressed++)) << 16;
        xor[j] |= ((uint64_t)(*compressed++)) << 8;
        xor[j] |= ((uint64_t)(*compressed++));
        break;
        }
        }
      }
    for (int j = 0; j < max_j; ++j)
      {
      if (bcode[j] > 8)
        prediction1 = prediction2;

      value = xor[j] ^ prediction1;

      hash_table_1[hash1] = value;
      hash1 = trico_compute_hash1_64(hash1, value, hash1_size_exponent, hash1_mask);
      prediction1 = hash_table_1[hash1];

      stride = value - last_value;
      hash_table_2[hash2] = stride;
      hash2 = trico_compute_hash2_64(hash2, stride, hash2_size_exponent, hash2_mask);
      prediction2 = value + hash_table_2[hash2];
      last_value = value;

      *p_out++ = value;
      }
    }

  free(hash_table_1);
  free(hash_table_2);
  }
  