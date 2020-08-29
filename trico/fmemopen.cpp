#include "fmemopen.h"

#ifdef _WIN32

#include <windows.h>

FILE *fmemopen(void *buf, size_t size, const char *mode) 
  {
  char temppath[MAX_PATH - 13];
  if (0 == GetTempPath(sizeof(temppath), temppath))
    return NULL;

  char filename[MAX_PATH + 1];
  if (0 == GetTempFileName(temppath, "SC", 0, filename))
    return NULL;

  FILE *f = fopen(filename, "wb");
  if (NULL == f)
    return NULL;

  fwrite(buf, size, 1, f);
  fclose(f);

  return fopen(filename, mode);
  }

#endif
