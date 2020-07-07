#pragma once

#if defined(_AVX2)  || defined(_SSE2)
#include <immintrin.h>
#include <emmintrin.h>
#endif

#include <stdlib.h>
#include <stdint.h>