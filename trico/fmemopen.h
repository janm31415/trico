#pragma once

#ifdef _WIN32

#include <stdio.h>
FILE* fmemopen(void *, size_t, const char *);

#endif