#ifndef TRICO_TRICO_API
#define TRICO_TRICO_API

#ifdef _WIN32
#if defined(TRICO_DLL_EXPORT) // Windows dll export
#  define TRICO_API __declspec(dllexport)
#elif defined(TRICO_DLL_IMPORT) // Windows dll import
#  define TRICO_API __declspec(dllimport)
#else // Windows static library
#  define TRICO_API
#endif
#else // #ifdef _WIN32
#define TRICO_API
#endif // #ifdef _WIN32

#endif // #ifndef TRICO_TRICO_API