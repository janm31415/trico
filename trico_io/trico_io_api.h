#ifndef TRICO_IO_TRICO_IO_API
#define TRICO_IO_TRICO_IO_API

#ifdef _WIN32
#if defined(TRICO_DLL_EXPORT) // Windows dll export
#  define TRICO_IO_API __declspec(dllexport)
#elif defined(TRICO_DLL_IMPORT) // Windows dll import
#  define TRICO_IO_API __declspec(dllimport)
#else // Windows static library
#  define TRICO_IO_API
#endif
#else // #ifdef _WIN32
#define TRICO_IO_API
#endif // #ifdef _WIN32

#endif // #ifndef TRICO_IO_TRICO_IO_API