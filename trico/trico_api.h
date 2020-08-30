#ifndef TRICO_TRICO_API
#define TRICO_TRICO_API

#ifdef _WIN32
#if defined(TRICO_DLL_EXPORT)
#  define TRICO_API __declspec(dllexport)
#elif defined(TRICO_DLL_IMPORT)
#  define TRICO_API __declspec(dllimport)
#else
#  define TRICO_API
#endif
#else
#define TRICO_API
#endif

#endif