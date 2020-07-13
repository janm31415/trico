#include "fpc.h"
#include "alloc.h"

#include <cassert>

namespace trico
  {

#define HASH_SIZE_EXPONENT 20
#define HASH_SIZE (1 << HASH_SIZE_EXPONENT)
#define HASH_MASK (HASH_SIZE - 1)


  static const long long mask[8] =
    { 0x0000000000000000LL,
     0x00000000000000ffLL,
     0x000000000000ffffLL,
     0x0000000000ffffffLL,
     0x000000ffffffffffLL,
     0x0000ffffffffffffLL,
     0x00ffffffffffffffLL,
     0xffffffffffffffffLL };

  void fpc_compress(uint32_t* nr_of_compressed_bytes, uint8_t** outbuffer, const double* input, const uint32_t number_of_doubles)
    {
    uint32_t max_size = (number_of_doubles) * sizeof(double) + 2 + 6 + (number_of_doubles / 2);
    *outbuffer = (uint8_t*)trico_malloc(max_size);

    uint8_t* outbuf = *outbuffer;

    long i, out, intot, hash, dhash, code, bcode, ioc;
    long long val, lastval, stride, pred1, pred2, xor1, xor2;
    long long *fcm, *dfcm;

    fcm = (long long*)calloc(HASH_SIZE, 8);
    dfcm = (long long*)calloc(HASH_SIZE, 8);

    hash = 0;
    dhash = 0;
    lastval = 0;
    pred1 = 0;
    pred2 = 0;

    intot = number_of_doubles;

    const long long* inbuf = reinterpret_cast<const long long*>(input);


    val = inbuf[0];
    out = 6 + ((intot + 1) >> 1);
    *((long long *)&outbuf[(out >> 3) << 3]) = 0;
    for (i = 0; i < intot; i += 2)
      {
      xor1 = val ^ pred1;
      fcm[hash] = val;
      hash = ((hash << 6) ^ ((unsigned long long)val >> 48)) & HASH_MASK;
      pred1 = fcm[hash];

      stride = val - lastval;
      xor2 = val ^ (lastval + pred2);
      lastval = val;
      val = inbuf[i + 1];
      dfcm[dhash] = stride;
      dhash = ((dhash << 2) ^ ((unsigned long long)stride >> 40)) & HASH_MASK;
      pred2 = dfcm[dhash];

      code = 0;
      if ((unsigned long long)xor1 > (unsigned long long)xor2)
        {
        code = 0x80;
        xor1 = xor2;
        }
      bcode = 7;                // 8 bytes
      if (0 == (xor1 >> 56))
        bcode = 6;              // 7 bytes
      if (0 == (xor1 >> 48))
        bcode = 5;              // 6 bytes
      if (0 == (xor1 >> 40))
        bcode = 4;              // 5 bytes
      if (0 == (xor1 >> 24))
        bcode = 3;              // 3 bytes
      if (0 == (xor1 >> 16))
        bcode = 2;              // 2 bytes
      if (0 == (xor1 >> 8))
        bcode = 1;              // 1 byte
      if (0 == xor1)
        bcode = 0;              // 0 bytes

      *((long long *)&outbuf[(out >> 3) << 3]) |= xor1 << ((out & 0x7) << 3);
      if (0 == (out & 0x7))
        xor1 = 0;
      *((long long *)&outbuf[((out >> 3) << 3) + 8]) = (unsigned long long)xor1 >> (64 - ((out & 0x7) << 3));

      out += bcode + (bcode >> 2);
      code |= bcode << 4;

      xor1 = val ^ pred1;
      fcm[hash] = val;
      hash = ((hash << 6) ^ ((unsigned long long)val >> 48)) & HASH_MASK;
      pred1 = fcm[hash];

      stride = val - lastval;
      xor2 = val ^ (lastval + pred2);
      lastval = val;
      val = inbuf[i + 2];
      dfcm[dhash] = stride;
      dhash = ((dhash << 2) ^ ((unsigned long long)stride >> 40)) & HASH_MASK;
      pred2 = dfcm[dhash];

      bcode = code | 0x8;
      if ((unsigned long long)xor1 > (unsigned long long)xor2)
        {
        code = bcode;
        xor1 = xor2;
        }
      bcode = 7;                // 8 bytes
      if (0 == (xor1 >> 56))
        bcode = 6;              // 7 bytes
      if (0 == (xor1 >> 48))
        bcode = 5;              // 6 bytes
      if (0 == (xor1 >> 40))
        bcode = 4;              // 5 bytes
      if (0 == (xor1 >> 24))
        bcode = 3;              // 3 bytes
      if (0 == (xor1 >> 16))
        bcode = 2;              // 2 bytes
      if (0 == (xor1 >> 8))
        bcode = 1;              // 1 byte
      if (0 == xor1)
        bcode = 0;              // 0 bytes

      *((long long *)&outbuf[(out >> 3) << 3]) |= xor1 << ((out & 0x7) << 3);
      if (0 == (out & 0x7))
        xor1 = 0;
      *((long long *)&outbuf[((out >> 3) << 3) + 8]) = (unsigned long long)xor1 >> (64 - ((out & 0x7) << 3));

      out += bcode + (bcode >> 2);
      outbuf[6 + (i >> 1)] = code | bcode;
      }
    if (0 != (intot & 1)) {
      out -= bcode + (bcode >> 2);
      }
    outbuf[0] = intot;
    outbuf[1] = intot >> 8;
    outbuf[2] = intot >> 16;
    outbuf[3] = out;
    outbuf[4] = out >> 8;
    outbuf[5] = out >> 16;

    *nr_of_compressed_bytes = out;

    free(fcm);
    free(dfcm);
    }

  void fpc_decompress(uint32_t* number_of_doubles, double** uncompressed, const uint8_t* compressed)
    {
    long i, in, intot, hash, dhash, code, bcode, end, tmp, ioc;
    long long val, lastval, stride, pred1, pred2, next;
    long long *fcm, *dfcm;

    fcm = (long long*)calloc(HASH_SIZE, 8);
    dfcm = (long long*)calloc(HASH_SIZE, 8);

    hash = 0;
    dhash = 0;
    lastval = 0;
    pred1 = 0;
    pred2 = 0;

    const uint8_t* inbuf = compressed;

    intot = inbuf[2];
    intot = (intot << 8) | inbuf[1];
    intot = (intot << 8) | inbuf[0];
    in = inbuf[5];
    in = (in << 8) | inbuf[4];
    in = (in << 8) | inbuf[3];

    *number_of_doubles = intot;
    *uncompressed = (double*)trico_malloc(*number_of_doubles * sizeof(double));

    long long* outbuf = reinterpret_cast<long long*>(*uncompressed);

    inbuf += 6;
    in = (intot + 1) >> 1;
    for (i = 0; i < intot; i += 2)
      {
      code = inbuf[i >> 1];

      val = *((long long *)&inbuf[(in >> 3) << 3]);
      next = *((long long *)&inbuf[((in >> 3) << 3) + 8]);
      tmp = (in & 0x7) << 3;
      val = (unsigned long long)val >> tmp;
      next <<= 64 - tmp;
      if (0 == tmp)
        next = 0;
      val |= next;

      bcode = (code >> 4) & 0x7;
      val &= mask[bcode];
      in += bcode + (bcode >> 2);

      if (0 != (code & 0x80))
        pred1 = pred2;
      val ^= pred1;

      fcm[hash] = val;
      hash = ((hash << 6) ^ ((unsigned long long)val >> 48)) & HASH_MASK;
      pred1 = fcm[hash];

      stride = val - lastval;
      dfcm[dhash] = stride;
      dhash = ((dhash << 2) ^ ((unsigned long long)stride >> 40)) & HASH_MASK;
      pred2 = val + dfcm[dhash];
      lastval = val;

      outbuf[i] = val;

      val = *((long long *)&inbuf[(in >> 3) << 3]);
      next = *((long long *)&inbuf[((in >> 3) << 3) + 8]);
      tmp = (in & 0x7) << 3;
      val = (unsigned long long)val >> tmp;
      next <<= 64 - tmp;
      if (0 == tmp)
        next = 0;
      val |= next;

      bcode = code & 0x7;
      val &= mask[bcode];
      in += bcode + (bcode >> 2);

      if (0 != (code & 0x8))
        pred1 = pred2;
      val ^= pred1;

      fcm[hash] = val;
      hash = ((hash << 6) ^ ((unsigned long long)val >> 48)) & HASH_MASK;
      pred1 = fcm[hash];

      stride = val - lastval;
      dfcm[dhash] = stride;
      dhash = ((dhash << 2) ^ ((unsigned long long)stride >> 40)) & HASH_MASK;
      pred2 = val + dfcm[dhash];
      lastval = val;

      outbuf[i + 1] = val;
      }

    free(fcm);
    free(dfcm);
    }
  }